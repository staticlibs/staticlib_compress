Staticlibs Compress library
===========================

[![travis](https://travis-ci.org/staticlibs/staticlib_compress.svg?branch=master)](https://travis-ci.org/staticlibs/staticlib_compress)
[![appveyor](https://ci.appveyor.com/api/projects/status/github/staticlibs/staticlib_compress?svg=true)](https://ci.appveyor.com/project/staticlibs/staticlib-compress)

This project is a part of [Staticlibs](http://staticlibs.net/).

This project provides implementation of `Source` (input stream) and `Sink` (output stream)
that performs compression/decompress using [Zlib](http://www.zlib.net/) and [Xz](http://tukaani.org/xz/)
compression algorithms.

It additionally provides `zip_sink` that allows to write ZIP files. [staticlib_unzip](https://github.com/staticlibs/staticlib_unzip) library can be used to read ZIP files.

This library is header-only and depends on [staticlib_io](https://github.com/staticlibs/staticlib_io.git),
[staticlib_config](https://github.com/staticlibs/staticlib_config.git),
Zlib and Xz Utils (liblzma).

Link to the [API documentation](http://staticlibs.github.io/staticlib_compress/docs/html/namespacestaticlib_1_1compress.html).

License information
-------------------

This project is released under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0).

Changelog
---------

**2018-05-16**

 * version 1.2.2
 * drop broken move constructor in `zip_sink`

**2018-02-22**

 * version 1.2.1
 * check data avail before writing

**2017-12-24**

 * version 1.2.0
 * `zip_sink` implementation
 * vs2017 support

**2017-05-29**

 * version 1.1.0
 * update interfaces to use spans

**2016-02-06**

 * version 1.0
