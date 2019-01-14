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
zlib test file generator.

Generates zlib test files with specified fields in the header.
"""

import argparse
import struct
import zlib
import textwrap

def write_zlib(f, args):
    """Write zlib data to file f."""

    # zlib header
    data = bytearray.fromhex('00 00')

    cmf = 8
    flg = 0

    if args.window_size:
        cmf |= args.window_size << 4

    if args.level:
        flg = args.level << 6

    if args.preset_dict:
        flg |= 1 << 5
        # Write Adler-32 of preset dictionary
        data.extend(struct.pack('>I', args.preset_dict))

    flg |= 31 - ((256 * cmf + flg) % 31)

    data[0] = cmf
    data[1] = flg

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
        # Adler-32 of empty buffer
        data.extend(struct.pack('>I', 1))
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
        # Adler-32 of one byte 00
        data.extend(struct.pack('>I', zlib.adler32(b'\x00')))

    if args.i:
        s = textwrap.fill(', '.join('0x{:02X}'.format(b) for b in data))
        f.write(s.encode('utf-8'))
    else:
        f.write(data)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='zlib test file generator.')
    parser.add_argument('-d', '--data', type=str,
                        choices=['empty', 'byte00'], default='empty',
                        help='compressed data')
    parser.add_argument('-m', '--method', type=str,
                        choices=['raw', 'fixed', 'dynamic'], default='fixed',
                        help='compression method')
    parser.add_argument('-l', '--level', type=int,
                        choices=range(0, 4), default=2,
                        help='compression level')
    parser.add_argument('-w', '--window_size', type=int,
                        choices=range(0, 8), default=7,
                        help='window size')
    parser.add_argument('-p', '--preset_dict', type=int,
                        help='preset dictionary Adler-32')
    parser.add_argument('-i', action='store_true', help='generate include file')
    parser.add_argument('outfile', type=argparse.FileType('wb'), help='output file')
    args = parser.parse_args()

    write_zlib(args.outfile, args)
