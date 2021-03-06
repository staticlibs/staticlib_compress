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
 * File:   lzma_source.hpp
 * Author: alex
 *
 * Created on January 29, 2016, 9:21 PM
 */

#ifndef STATICLIB_COMPRESS_LZMA_SOURCE_HPP
#define STATICLIB_COMPRESS_LZMA_SOURCE_HPP

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <memory>
#include <type_traits>

#include "lzma.h"

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"

#include "staticlib/compress/compress_exception.hpp"


namespace staticlib {
namespace compress {

/**
 * Source wrapper that decompresses deflated data
 */
template <typename Source, std::size_t buf_size = 4096 >
class lzma_source {
    /**
     * Source of compressed data
     */
    Source src;
    /**
     * Internal buffer
     */
    std::array<char, buf_size> buf;
    /**
     * LZMA decompressing stream
     */
    lzma_stream* strm;
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
    lzma_source(Source src) :
    src(std::move(src)),
    strm([] {
        lzma_stream* stream = static_cast<lzma_stream*> (std::malloc(sizeof(lzma_stream)));
        if (nullptr == stream) throw compress_exception(TRACEMSG(
                "Error creating lzma stream: 'malloc' failed"));
        *stream = LZMA_STREAM_INIT;
        auto err = lzma_stream_decoder(stream, UINT64_MAX, 0);
        if (LZMA_OK != err) throw compress_exception(TRACEMSG(
                "Error initializing LZMA stream, code: [" + sl::support::to_string(err) + "]"));
        return stream;
    }()) { }

    ~lzma_source() STATICLIB_NOEXCEPT {
        if (nullptr == strm) return;
        ::lzma_end(strm);
        std::free(strm);
    }

    /**
     * Deleted copy constructor
     * 
     * @param other instance
     */
    lzma_source(const lzma_source&) = delete;

    /**
     * Deleted copy assignment operator
     * 
     * @param other instance
     * @return this instance 
     */
    lzma_source& operator=(const lzma_source&) = delete;

    /**
     * Move constructor
     * 
     * @param other other instance
     */
    lzma_source(lzma_source&& other) :
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
    lzma_source& operator=(lzma_source&& other) {
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
            strm->next_in = reinterpret_cast<uint8_t*> (buf.data() + pos);
            strm->avail_in = avail;
            strm->next_out = reinterpret_cast<uint8_t*> (span.data());
            strm->avail_out = static_cast<size_t> (span.size());
            // call inflate
            auto err = ::lzma_code(strm, LZMA_RUN);
            if (LZMA_OK == err || LZMA_STREAM_END == err) {
                std::streamsize read = avail - strm->avail_in;
                std::streamsize written = span.size_signed() - strm->avail_out;
                size_t uread = static_cast<size_t> (read);
                pos += uread;
                avail -= uread;
                if (written > 0 || LZMA_STREAM_END != err) {
                    return written;
                }
                exhausted = true;
                return std::char_traits<char>::eof();
            } else throw compress_exception(TRACEMSG(
                    + "LZMA error, code: [" + sl::support::to_string(err) + "]"));
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
 * Factory function for creating lzma sources,
 * created object will own the specified source
 * 
 * @param source input source
 * @return lzma source
 */
template <typename Source,
class = typename std::enable_if<!std::is_lvalue_reference<Source>::value>::type>
lzma_source<Source> make_lzma_source(Source&& source) {
    return lzma_source<Source>(std::move(source));
}

/**
 * Factory function for creating lzma sources,
 * created object will NOT own the specified source
 * 
 * @param source input source
 * @return lzma source
 */
template <typename Source>
lzma_source<sl::io::reference_source<Source>> make_lzma_source(Source& source) {
    return lzma_source<sl::io::reference_source<Source>> (
            sl::io::make_reference_source(source));
}

} // namespace
}


#endif /* STATICLIB_COMPRESS_LZMA_SOURCE_HPP */

