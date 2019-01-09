
tinf - tiny inflate library
===========================

Version 1.0.0

Copyright (c) 2003-2019 Joergen Ibsen

<http://www.ibsensoftware.com/>

[![Build Status](https://dev.azure.com/jibsenorg/jibsen/_apis/build/status/jibsen.tinf?branchName=master)](https://dev.azure.com/jibsenorg/jibsen/_build/latest?definitionId=3?branchName=master)

About
-----

tinf is a small library implementing the decompression algorithm for the
[deflate][wpdeflate] compressed data format (called 'inflate'). Deflate
compression is used in e.g. zlib, gzip, zip, and png.

I wrote it because I needed a small in-memory zlib decompressor for a self-
extracting archive, and the zlib library added 15k to my program. The tinf
code added only 2k.

Naturally the size difference is insignificant in most cases. Also, the
zlib library has many more features, is more secure, and mostly faster.
But if you have a project that calls for a small and simple deflate
decompressor, give it a try :-)

[wpdeflate]: https://en.wikipedia.org/wiki/DEFLATE


Usage
-----

The include file `src/tinf.h` contains documentation in the form of
[doxygen][] comments.

Wrappers for decompressing zlib and gzip data in memory are supplied.

tgunzip, an example command-line gzip decompressor in C, is included.

tinf uses [CMake][] to generate build systems. To create one for the tools on
your platform, and build tinf, use something along the lines of:

~~~sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
~~~

You can also simply compile the source files and link them into your project.
CMake just provides an easy way to build and test across various platforms and
toolsets.

[doxygen]: http://www.doxygen.org/
[CMake]: http://www.cmake.org/


Notes
-----

While the implementation should be fairly compliant, it does assume it is
given valid compressed data, and that there is sufficient space for the
decompressed data. This means it is **important** to only use tinf to
decompress data from trusted sources.

tinf requires int to be at least 32-bit.

The inflate algorithm and data format are from 'DEFLATE Compressed Data
Format Specification version 1.3' ([RFC 1951][deflate]).

The zlib data format is from 'ZLIB Compressed Data Format Specification
version 3.3' ([RFC 1950][zlib]).

The gzip data format is from 'GZIP file format specification version 4.3'
([RFC 1952][gzip]).

Ideas for future versions:

  - More safety checks
  - Memory for the `tinf_data` object should be passed, to avoid using more
    than 1k of stack space
  - Wrappers for unpacking zip archives and png images
  - Blocking of some sort, so everything does not have to be in memory
  - Optional table-based Huffman decoder

[deflate]: http://www.rfc-editor.org/rfc/rfc1951.txt
[zlib]: http://www.rfc-editor.org/rfc/rfc1950.txt
[gzip]: http://www.rfc-editor.org/rfc/rfc1952.txt


Related Projects
----------------

  - [puff](https://github.com/madler/zlib) (in the contrib folder of zlib)
  - [tinfl](https://github.com/richgel999/miniz) (part of miniz)
  - [uzlib](https://github.com/pfalcon/uzlib)
  - [gdunzip](https://github.com/jellehermsen/gdunzip) (GDScript)
  - [tiny-inflate](https://github.com/foliojs/tiny-inflate) (JavaScript)
  - [tinflate](http://achurch.org/tinflate.c) (unrelated to this project)
  - The [Wikipedia page for deflate](https://en.wikipedia.org/wiki/DEFLATE)
    has a list of implementations


License
-------

This projected is licensed under the [zlib License](LICENSE) (Zlib).
