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
 * File:   lzma_sink_test.test
 * Author: alex
 *
 * Created on February 6, 2016, 3:17 PM
 */

#include "staticlib/compress/lzma_sink.hpp"

#include <array>
#include <iostream>

#include "staticlib/config/assert.hpp"
#include "staticlib/io.hpp"
#include "staticlib/tinydir.hpp"

void test_lzma() {
    auto fd_comp = sl::tinydir::file_source("../test/data/hello.txt.xz");
    auto ss_comp = sl::io::string_sink();
    sl::io::copy_all(fd_comp, ss_comp);

    auto fd = sl::tinydir::file_source("../test/data/hello.txt");
    auto ss = sl::io::string_sink();
    {
        auto coder = sl::compress::make_lzma_sink(ss);
        sl::io::copy_all(fd, coder);
    }

    slassert(ss_comp.get_string() == ss.get_string());
}

void test_huge() {
    auto fd_in = sl::tinydir::file_source("/home/alex/ebook/maugham/bondage.txt");
    auto coder = sl::compress::make_lzma_sink(sl::tinydir::file_sink("bondage.txt.xz"));
    sl::io::copy_all(fd_in, coder);
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

