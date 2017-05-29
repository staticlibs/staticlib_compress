/*
 * Copyright 2016, alex at staticlibs.net
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * File:   inflate_source_test.cpp
 * Author: alex
 *
 * Created on February 4, 2016, 3:17 PM
 */

#include "staticlib/compress/inflate_source.hpp"

#include <array>
#include <iostream>

#include "staticlib/config/assert.hpp"
#include "staticlib/io.hpp"
#include "staticlib/tinydir.hpp"

void test_inflate() {
    auto fd = sl::tinydir::file_source("../test/data/hello.txt.deflate");
    auto inflater = sl::compress::make_inflate_source(fd);
    auto ss = sl::io::string_sink();
    sl::io::copy_all(inflater, ss);
    slassert("hello" == ss.get_string());
}

void test_huge() {
    auto inflater = sl::compress::make_inflate_source(sl::tinydir::file_source("winxp_printer.vdi.deflate"));
    auto fd_out = sl::tinydir::file_sink("winxp_printer.vdi");
    sl::io::copy_all(inflater, fd_out);
}

int main() {
    try {
        test_inflate();
//        test_huge();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}
