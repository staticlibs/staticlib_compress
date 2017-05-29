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
 * File:   zip_sink_test.cpp
 * Author: alex
 *
 * Created on February 13, 2016, 8:16 PM
 */

#include "staticlib/compress/zip_sink.hpp"

#include <array>
#include <iostream>

#include "staticlib/config/assert.hpp"
#include "staticlib/io.hpp"
#include "staticlib/tinydir.hpp"

void test_store() {
    auto sink = sl::compress::make_zip_sink(sl::tinydir::file_sink("test.zip"));
    sink.add_entry("foo.txt");
    sink.write({"hello", 5});
    sink.add_entry("bar/baz.txt");
    sink.write({"bye", 3});
}

int main() {
    try {
        test_store();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}

