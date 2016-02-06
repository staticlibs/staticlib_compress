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
#include "staticlib/utils/FileDescriptor.hpp"
#include "staticlib/io.hpp"

namespace su = staticlib::utils;
namespace si = staticlib::io;
namespace sc = staticlib::compress;

void test_lzma() {
    su::FileDescriptor fd{"../test/data/hello.txt.xz", 'r'};
    auto coder = sc::make_lzma_source(fd);
    si::string_sink ss{};
    std::array<char, 4096> buf;
    si::copy_all(coder, ss, buf.data(), buf.size());
    slassert("hello" == ss.get_string());
}

void test_huge() {
    std::array<char, 4096> buf;

    auto inflater = sc::make_lzma_source(su::FileDescriptor{"bondage.txt.xz", 'r'});
    su::FileDescriptor fd_out{"bondage.txt", 'w'};
    si::copy_all(inflater, fd_out, buf.data(), buf.size());
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


