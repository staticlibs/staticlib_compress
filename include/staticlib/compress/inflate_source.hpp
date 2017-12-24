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
 * File:   inflate_source.hpp
 * Author: alex
 *
 * Created on January 29, 2016, 9:19 PM
 */

#ifndef STATICLIB_COMPRESS_INFLATE_SOURCE_HPP
#define STATICLIB_COMPRESS_INFLATE_SOURCE_HPP

#include <cstdint>
#include <cstring>
#include <ios>
#include <memory>
#include <type_traits>

#include "zlib.h"

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"

#include "staticlib/compress/compress_exception.hpp"


namespace staticlib {
namespace compress {

/**
 * Source wrapper that decompresses deflated data
 */
template <typename Source, std::size_t buf_size = 4096>
class inflate_source {
    /**
     * Source of compressed data
     */
    Source src;
    /**
     * Internal buffer
     */
    std::array<char, buf_size> buf;
    /**
     * Zlib decompressing stream
     */
    z_stream* strm;
    /**
     * Start position in internal buffer
     */
    size_t pos = 0;
    /**
     * Number of bytes available in internal buffer
     */
    size_t avail = 0;
    /**
     * Source EOF flag
     */
    bool exhausted = false;

public:
    /**
     * Constructor, created object will own the specified source
     * 
     * @param src source to read compressed data from
     */
    inflate_source(Source src) :
    src(std::move(src)),
    strm([]{
        z_stream* stream = static_cast<z_stream*> (std::malloc(sizeof(z_stream)));
        if (nullptr == stream) throw compress_exception(TRACEMSG(
                "Error creating inflate stream: 'malloc' failed"));
        std::memset(stream, 0, sizeof (z_stream));
        auto err = inflateInit2(stream, -MAX_WBITS);
        if (Z_OK != err) throw compress_exception(TRACEMSG(
                "Error initializing inflate stream: [" + ::zError(err) + "]"));
        return stream;
    }()) { }

    ~inflate_source() STATICLIB_NOEXCEPT {
        if (nullptr == strm) return;
        ::inflateEnd(strm);
        std::free(strm);
    }

    /**
     * Deleted copy constructor
     * 
     * @param other instance
     */
    inflate_source(const inflate_source&) = delete;

    /**
     * Deleted copy assignment operator
     * 
     * @param other instance
     * @return this instance 
     */
    inflate_source& operator=(const inflate_source&) = delete;

    /**
     * Move constructor
     * 
     * @param other other instance
     */
    inflate_source(inflate_source&& other) :
    src(std::move(other.src)),
    buf(std::move(other.buf)),
    strm(other.strm),
    pos(other.pos),
    avail(other.avail),
    exhausted(other.exhausted) {
        other.strm = nullptr;
    }

    /**
     * Move assignment operator
     * 
     * @param other other instance
     * @return this instance
     */
    inflate_source& operator=(inflate_source&& other) {
        src = std::move(other.src);
        buf = std::move(other.buf);
        strm = other.strm;
        other.strm = nullptr;
        pos = other.pos;
        avail = other.avail;
        exhausted = other.exhausted;
        return *this;
    }

    /**
     * Read implementation
     * 
     * @param span output span
     * @return number of bytes written into specified span
     */
    std::streamsize read(sl::io::span<char> span) {
        if (!exhausted) {
            // fill buffer if empty
            if (0 == avail) {
                avail = sl::io::read_all(src, {buf.data(), buf.size()});
                pos = 0;
            }
            // prepare zlib stream
            strm->next_in = reinterpret_cast<unsigned char*> (buf.data() + pos);
            strm->avail_in = static_cast<uInt> (avail);
            strm->next_out = reinterpret_cast<unsigned char*> (span.data());
            strm->avail_out = static_cast<uInt> (span.size());
            // call inflate
            auto err = ::inflate(strm, Z_FINISH);
            if (Z_OK == err || Z_STREAM_END == err || Z_BUF_ERROR == err) {
                std::streamsize read = avail - strm->avail_in;
                std::streamsize written = span.size_signed() - strm->avail_out;
                size_t uread = static_cast<size_t> (read);
                pos += uread;
                avail -= uread;
                if (written > 0 || Z_STREAM_END != err) {
                    return written;
                }
                exhausted = true;
                return std::char_traits<char>::eof();
            } else throw compress_exception(TRACEMSG(
                    + "Inflate error: [" + ::zError(err) + "]"));
        } else {
            return std::char_traits<char>::eof();
        }
    }
    
    /**
     * Underlying source accessor
     * 
     * @return underlying source reference
     */
    Source& get_source() {
        return src;
    }  
    
};

/**
 * Factory function for creating inflate sources,
 * created object will own the specified source
 * 
 * @param source input source
 * @return inflate source
 */
template <typename Source, 
        class = typename std::enable_if<!std::is_lvalue_reference<Source>::value>::type>
inflate_source<Source> make_inflate_source(Source&& source) {
    return inflate_source<Source>(std::move(source));
}

/**
 * Factory function for creating inflate sources,
 * created object will NOT own the specified source
 * 
 * @param source input source
 * @return inflate source
 */
template <typename Source>
inflate_source<sl::io::reference_source<Source>> make_inflate_source(Source& source) {
    return inflate_source<sl::io::reference_source<Source>>(
            sl::io::make_reference_source(source));
}

} // namespace
}

#endif /* STATICLIB_COMPRESS_INFLATE_SOURCE_HPP */
