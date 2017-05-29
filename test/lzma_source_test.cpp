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
 * File:   lzma_source_test.cpp
 * Author: alex
 *
 * Created on February 6, 2016, 3:40 PM
 */

#include "staticlib/compress/lzma_source.hpp"

#include <array>
#include <iostream>

#include "staticlib/config/assert.hpp"
#include "staticlib/io.hpp"
#include "staticlib/tinydir.hpp"

void test_lzma() {
    auto fd = sl::tinydir::file_source("../test/data/hello.txt.xz");
    auto coder = sl::compress::make_lzma_source(fd);
    auto ss = sl::io::string_sink();
    sl::io::copy_all(coder, ss);
    slassert("hello" == ss.get_string());
}

void test_huge() {
    auto inflater = sl::compress::make_lzma_source(sl::tinydir::file_source("bondage.txt.xz"));
    auto fd_out = sl::tinydir::file_sink("bondage.txt");
    sl::io::copy_all(inflater, fd_out);
}

int main() {
    try {
        test_lzma();
//        test_huge();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}


