/*
 * Copyright 2017, alex at staticlibs.net
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
 * File:   zip_compression_method.hpp
 * Author: alex
 *
 * Created on December 3, 2017, 5:14 PM
 */

#ifndef STATICLIB_COMPRESS_ZIP_COMPRESSION_METHOD_HPP
#define STATICLIB_COMPRESS_ZIP_COMPRESSION_METHOD_HPP

#include <cstdint>

namespace staticlib {
namespace compress {

    enum class zip_compression_method : uint16_t {
        store = 0,
        deflate = 0x8
    };

} // namespace
}

#endif /* STATICLIB_COMPRESS_ZIP_COMPRESSION_METHOD_HPP */

