#!/usr/bin/env python3

# Copyright (c) 2014-2019 Joergen Ibsen
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

"""
Gzip test file generator.

Generates gzip test files with specified fields in the header.
"""

import argparse
import struct
import zlib
import textwrap

def write_gzip(f, args):
    """Write gzip data to file f."""

    # gzip header
    data = bytearray.fromhex('1f 8b 08 00 00 00 00 00 02 0b')

    flags = 0

    if args.s:
        flags |= 2

    if args.extra:
        flags |= 4
        edata = args.extra.encode('ascii')
        data.extend(struct.pack('<H', len(edata)))
        data.extend(edata)

    if args.filename:
        flags |= 8
        data.extend(args.filename.encode('ascii'))
        data.extend(b'\x00')

    if args.comment:
        flags |= 16
        data.extend(args.comment.encode('ascii'))
        data.extend(b'\x00')

    data[3] = flags

    if args.s:
        # write low two bytes of CRC32 of header up to here
        data.extend(struct.pack('<H', zlib.crc32(data) & 0xffff))

    if args.data == 'empty':
        if args.method == 'raw':
            # empty buffer, uncompressed
            data.extend(bytes.fromhex('01 00 00 ff ff'))
        elif args.method == 'fixed':
            # empty buffer, fixed Huffman
            data.extend(bytes.fromhex('03 00'))
        elif args.method == 'dynamic':
            # empty buffer, dynamic Huffman
            data.extend(bytes.fromhex('05 c1 81 00 00 00 00 00 10 ff d5 08'))
        # CRC32 of empty buffer
        data.extend(struct.pack('<I', 0))
        # ISIZE 0
        data.extend(struct.pack('<I', 0))
    elif args.data == 'byte00':
        if args.method == 'raw':
            # one byte 00, uncompressed
            data.extend(bytes.fromhex('01 01 00 fe ff 00'))
        elif args.method == 'fixed':
            # one byte 00, fixed Huffman
            data.extend(bytes.fromhex('63 00 00'))
        elif args.method == 'dynamic':
            # one byte 00, dynamic Huffman
            data.extend(bytes.fromhex('05 c1 81 00 00 00 00 00 10 ff d5 10'))
        # CRC32 of one byte 00
        data.extend(struct.pack('<I', zlib.crc32(b'\x00')))
        # ISIZE 1
        data.extend(struct.pack('<I', 1))

    if args.i:
        s = textwrap.fill(', '.join('0x{:02X}'.format(b) for b in data))
        f.write(s.encode('utf-8'))
    else:
        f.write(data)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='gzip test file generator.')
    parser.add_argument('-f', '--filename', type=str, help='original filename')
    parser.add_argument('-c', '--comment', type=str, help='file comment')
    parser.add_argument('-e', '--extra', type=str, help='extra data')
    parser.add_argument('-d', '--data', type=str,
                        choices=['empty', 'byte00'], default='empty',
                        help='compressed data')
    parser.add_argument('-m', '--method', type=str,
                        choices=['raw', 'fixed', 'dynamic'], default='fixed',
                        help='compression method')
    parser.add_argument('-s', action='store_true', help='add header CRC16')
    parser.add_argument('-i', action='store_true', help='generate include file')
    parser.add_argument('outfile', type=argparse.FileType('wb'), help='output file')
    args = parser.parse_args()

    write_gzip(args.outfile, args)
