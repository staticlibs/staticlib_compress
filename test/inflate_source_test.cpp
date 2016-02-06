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
#include "staticlib/utils/FileDescriptor.hpp"
#include "staticlib/io.hpp"

namespace su = staticlib::utils;
namespace si = staticlib::io;
namespace sc = staticlib::compress;

void test_inflate() {
    su::FileDescriptor fd{"../test/data/hello.txt.deflate", 'r'};
    auto inflater = sc::make_inflate_source(fd);
    si::string_sink ss{};
    std::array<char, 4096> buf;
    si::copy_all(inflater, ss, buf.data(), buf.size());
    slassert("hello" == ss.get_string());
}

void test_huge() {
    std::array<char, 4096> buf;

    auto inflater = sc::make_inflate_source(su::FileDescriptor{"winxp_printer.vdi.deflate", 'r'});
    su::FileDescriptor fd_out{"winxp_printer.vdi", 'w'};
    si::copy_all(inflater, fd_out, buf.data(), buf.size());
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
