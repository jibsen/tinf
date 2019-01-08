/*
 * tinflate - tiny inflate
 *
 * Copyright (c) 2003-2019 Joergen Ibsen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must
 *      not claim that you wrote the original software. If you use this
 *      software in a product, an acknowledgment in the product
 *      documentation would be appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must
 *      not be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source
 *      distribution.
 */

#include "tinf.h"

#include <assert.h>
#include <limits.h>

#if defined(UINT_MAX) && (UINT_MAX) < 0xFFFFFFFFUL
#  error "tinf requires unsigned int to be at least 32-bit"
#endif

/* ------------------------------ *
 * -- internal data structures -- *
 * ------------------------------ */

struct tinf_tree {
	unsigned short table[16]; /* table of code length counts */
	unsigned short trans[288]; /* code -> symbol translation table */
};

struct tinf_data {
	const unsigned char *source;
	unsigned int tag;
	int bitcount;

	unsigned char *dest;
	unsigned int *destLen;

	struct tinf_tree ltree; /* dynamic length/symbol tree */
	struct tinf_tree dtree; /* dynamic distance tree */
};

/* ----------------------- *
 * -- utility functions -- *
 * ----------------------- */

static unsigned int read_le16(const unsigned char *p)
{
	return ((unsigned int) p[0])
	     | ((unsigned int) p[1] << 8);
}

/* build the fixed huffman trees */
static void tinf_build_fixed_trees(struct tinf_tree *lt, struct tinf_tree *dt)
{
	int i;

	/* build fixed length tree */
	for (i = 0; i < 7; ++i) {
		lt->table[i] = 0;
	}

	lt->table[7] = 24;
	lt->table[8] = 152;
	lt->table[9] = 112;

	for (i = 0; i < 24; ++i) {
		lt->trans[i] = 256 + i;
	}
	for (i = 0; i < 144; ++i) {
		lt->trans[24 + i] = i;
	}
	for (i = 0; i < 8; ++i) {
		lt->trans[24 + 144 + i] = 280 + i;
	}
	for (i = 0; i < 112; ++i) {
		lt->trans[24 + 144 + 8 + i] = 144 + i;
	}

	/* build fixed distance tree */
	for (i = 0; i < 5; ++i) {
		dt->table[i] = 0;
	}

	dt->table[5] = 32;

	for (i = 0; i < 32; ++i) {
		dt->trans[i] = i;
	}
}

/* given an array of code lengths, build a tree */
static int tinf_build_tree(struct tinf_tree *t, const unsigned char *lengths,
                           unsigned int num)
{
	unsigned short offs[16];
	unsigned int i, sum;

	assert(num < 288);

	/* clear code length count table */
	for (i = 0; i < 16; ++i) {
		t->table[i] = 0;
	}

	/* scan symbol lengths, and sum code length counts */
	for (i = 0; i < num; ++i) {
		t->table[lengths[i]]++;
	}

	t->table[0] = 0;

	/* compute offset table for distribution sort */
	for (sum = 0, i = 0; i < 16; ++i) {
		offs[i] = sum;
		sum += t->table[i];
	}

	/* create code->symbol translation table (symbols sorted by code) */
	for (i = 0; i < num; ++i) {
		if (lengths[i]) {
			t->trans[offs[lengths[i]]++] = i;
		}
	}

	return TINF_OK;
}

/* ---------------------- *
 * -- decode functions -- *
 * ---------------------- */

static void tinf_refill(struct tinf_data *d, int num)
{
	assert(num >= 0 && num <= 32);

	/* read bytes until at least num bits available */
	while (d->bitcount < num) {
		d->tag |= (unsigned int) *d->source++ << d->bitcount;
		d->bitcount += 8;
	}

	assert(d->bitcount <= 32);
}

static unsigned int tinf_getbits_no_refill(struct tinf_data *d, int num)
{
	assert(num >= 0 && num <= d->bitcount);

	/* get bits from tag */
	unsigned int bits = d->tag & ((1UL << num) - 1);

	/* remove bits from tag */
	d->tag >>= num;
	d->bitcount -= num;

	return bits;
}

/* get num bits from source stream */
static unsigned int tinf_getbits(struct tinf_data *d, int num)
{
	tinf_refill(d, num);
	return tinf_getbits_no_refill(d, num);
}

/* read a num bit value from stream and add base */
static unsigned int tinf_getbits_base(struct tinf_data *d, int num, int base)
{
	return base + (num ? tinf_getbits(d, num) : 0);
}

/* given a data stream and a tree, decode a symbol */
static int tinf_decode_symbol(struct tinf_data *d, const struct tinf_tree *t)
{
	int sum = 0, cur = 0, len = 0;

	/* get more bits while code value is above sum */
	do {
		cur = 2 * cur + tinf_getbits(d, 1);

		++len;

		assert(len <= 15);

		sum += t->table[len];
		cur -= t->table[len];
	} while (cur >= 0);

	assert(sum + cur >= 0 && sum + cur < 288);

	return t->trans[sum + cur];
}

/* given a data stream, decode dynamic trees from it */
static int tinf_decode_trees(struct tinf_data *d, struct tinf_tree *lt,
                             struct tinf_tree *dt)
{
	unsigned char lengths[288 + 32];
	int res;

	/* special ordering of code length codes */
	static const unsigned char clcidx[19] = {
		16, 17, 18, 0,  8, 7,  9, 6, 10, 5,
		11,  4, 12, 3, 13, 2, 14, 1, 15
	};
	unsigned int hlit, hdist, hclen;
	unsigned int i, num, length;

	/* get 5 bits HLIT (257-286) */
	hlit = tinf_getbits_base(d, 5, 257);

	/* get 5 bits HDIST (1-32) */
	hdist = tinf_getbits_base(d, 5, 1);

	/* get 4 bits HCLEN (4-19) */
	hclen = tinf_getbits_base(d, 4, 4);

	for (i = 0; i < 19; ++i) {
		lengths[i] = 0;
	}

	/* read code lengths for code length alphabet */
	for (i = 0; i < hclen; ++i) {
		/* get 3 bits code length (0-7) */
		unsigned int clen = tinf_getbits(d, 3);

		lengths[clcidx[i]] = clen;
	}

	/* build code length tree (in literal/length tree to save space) */
	res = tinf_build_tree(lt, lengths, 19);
	if (res != TINF_OK) {
		return res;
	}

	/* decode code lengths for the dynamic trees */
	for (num = 0; num < hlit + hdist; ) {
		int sym = tinf_decode_symbol(d, lt);

		switch (sym) {
		case 16:
			/* copy previous code length 3-6 times (read 2 bits) */
			sym = lengths[num - 1];
			length = tinf_getbits_base(d, 2, 3);
		    break;
		case 17:
			/* repeat code length 0 for 3-10 times (read 3 bits) */
			sym = 0;
			length = tinf_getbits_base(d, 3, 3);
			break;
		case 18:
			/* repeat code length 0 for 11-138 times (read 7 bits) */
			sym = 0;
			length = tinf_getbits_base(d, 7, 11);
			break;
		default:
			/* values 0-15 represent the actual code lengths */
			length = 1;
			break;
		}

		while (length--) {
			lengths[num++] = sym;
		}
	}

	/* build dynamic trees */
	res = tinf_build_tree(lt, lengths, hlit);
	if (res != TINF_OK) {
		return res;
	}
	res = tinf_build_tree(dt, lengths + hlit, hdist);
	if (res != TINF_OK) {
		return res;
	}

	return TINF_OK;
}

/* ----------------------------- *
 * -- block inflate functions -- *
 * ----------------------------- */

/* given a stream and two trees, inflate a block of data */
static int tinf_inflate_block_data(struct tinf_data *d, struct tinf_tree *lt,
                                   struct tinf_tree *dt)
{
	/* extra bits and base tables for length codes */
	static const unsigned char length_bits[30] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
		1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
		4, 4, 4, 4, 5, 5, 5, 5, 0, 127
	};

	static const unsigned short length_base[30] = {
		 3,  4,  5,   6,   7,   8,   9,  10,  11,  13,
		15, 17, 19,  23,  27,  31,  35,  43,  51,  59,
		67, 83, 99, 115, 131, 163, 195, 227, 258,   0
	};

	/* extra bits and base tables for distance codes */
	static const unsigned char dist_bits[30] = {
		0, 0,  0,  0,  1,  1,  2,  2,  3,  3,
		4, 4,  5,  5,  6,  6,  7,  7,  8,  8,
		9, 9, 10, 10, 11, 11, 12, 12, 13, 13
	};

	static const unsigned short dist_base[30] = {
		   1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
		  33,   49,   65,   97,  129,  193,  257,   385,   513,   769,
		1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
	};

	/* remember current output position */
	unsigned char *start = d->dest;

	for (;;) {
		int sym = tinf_decode_symbol(d, lt);

		/* check for end of block */
		if (sym == 256) {
			*d->destLen += d->dest - start;
			return TINF_OK;
		}

		if (sym < 256) {
			*d->dest++ = sym;
		}
		else {
			int length, dist, offs;
			int i;

			sym -= 257;

			/* possibly get more bits from length code */
			length = tinf_getbits_base(d, length_bits[sym],
			                        length_base[sym]);

			dist = tinf_decode_symbol(d, dt);

			/* possibly get more bits from distance code */
			offs = tinf_getbits_base(d, dist_bits[dist],
			                      dist_base[dist]);

			/* copy match */
			for (i = 0; i < length; ++i) {
				d->dest[i] = d->dest[i - offs];
			}

			d->dest += length;
		}
	}
}

/* inflate an uncompressed block of data */
static int tinf_inflate_uncompressed_block(struct tinf_data *d)
{
	unsigned int length, invlength;
	unsigned int i;

	/* get length */
	length = read_le16(d->source);

	/* get one's complement of length */
	invlength = read_le16(d->source + 2);

	/* check length */
	if (length != (~invlength & 0x0000FFFF)) {
		return TINF_DATA_ERROR;
	}

	d->source += 4;

	/* copy block */
	for (i = length; i; --i) {
		*d->dest++ = *d->source++;
	}

	/* make sure we start next block on a byte boundary */
	d->tag = 0;
	d->bitcount = 0;

	*d->destLen += length;

	return TINF_OK;
}

/* inflate a block of data compressed with fixed huffman trees */
static int tinf_inflate_fixed_block(struct tinf_data *d)
{
	/* build fixed huffman trees */
	tinf_build_fixed_trees(&d->ltree, &d->dtree);
	/* decode block using fixed trees */
	return tinf_inflate_block_data(d, &d->ltree, &d->dtree);
}

/* inflate a block of data compressed with dynamic huffman trees */
static int tinf_inflate_dynamic_block(struct tinf_data *d)
{
	/* decode trees from stream */
	int res = tinf_decode_trees(d, &d->ltree, &d->dtree);

	if (res != TINF_OK) {
		return res;
	}

	/* decode block using decoded trees */
	return tinf_inflate_block_data(d, &d->ltree, &d->dtree);
}

/* ---------------------- *
 * -- public functions -- *
 * ---------------------- */

/* initialize global (static) data */
void tinf_init()
{
	return;
}

/* inflate stream from source to dest */
int tinf_uncompress(void *dest, unsigned int *destLen,
                    const void *source, unsigned int sourceLen)
{
	struct tinf_data d;
	int bfinal;

	/* initialise data */
	d.source = (const unsigned char *) source;
	d.tag = 0;
	d.bitcount = 0;

	d.dest = (unsigned char *) dest;
	d.destLen = destLen;

	*destLen = 0;

	do {
		unsigned int btype;
		int res;

		/* read final block flag */
		bfinal = tinf_getbits(&d, 1);

		/* read block type (2 bits) */
		btype = tinf_getbits(&d, 2);

		/* decompress block */
		switch (btype) {
		case 0:
			/* decompress uncompressed block */
			res = tinf_inflate_uncompressed_block(&d);
			break;
		case 1:
			/* decompress block with fixed huffman trees */
			res = tinf_inflate_fixed_block(&d);
			break;
		case 2:
			/* decompress block with dynamic huffman trees */
			res = tinf_inflate_dynamic_block(&d);
			break;
		default:
			res = TINF_DATA_ERROR;
			break;
		}

		if (res != TINF_OK) {
			return res;
		}
	} while (!bfinal);

	return TINF_OK;
}
