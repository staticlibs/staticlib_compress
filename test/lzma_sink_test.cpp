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
#include "staticlib/utils/FileDescriptor.hpp"
#include "staticlib/io.hpp"

namespace su = staticlib::utils;
namespace si = staticlib::io;
namespace sc = staticlib::compress;

void test_lzma() {
    std::array<char, 4096> buf;

    su::FileDescriptor fd_comp{"../test/data/hello.txt.xz", 'r'};
    si::string_sink ss_comp{};
    si::copy_all(fd_comp, ss_comp, buf.data(), buf.size());

    su::FileDescriptor fd{"../test/data/hello.txt", 'r'};
    si::string_sink ss{};
    {
        auto coder = sc::make_lzma_sink(ss);
        si::copy_all(fd, coder, buf.data(), buf.size());
    }

    slassert(ss_comp.get_string() == ss.get_string());
}

void test_huge() {
    std::array<char, 4096> buf;

    su::FileDescriptor fd_in{"/home/alex/ebook/maugham/bondage.txt", 'r'};
    auto coder = sc::make_lzma_sink(su::FileDescriptor{"bondage.txt.xz", 'w'});
    si::copy_all(fd_in, coder, buf.data(), buf.size());
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

