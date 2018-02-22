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
 * File:   deflate_sink.hpp
 * Author: alex
 *
 * Created on January 29, 2016, 9:19 PM
 */

#ifndef STATICLIB_COMPRESS_DEFLATE_SINK_HPP
#define STATICLIB_COMPRESS_DEFLATE_SINK_HPP

#include <cstdlib>
#include <cstring>
#include <ios>
#include <memory>
#include <type_traits>

#include "zlib.h"

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/support.hpp"

#include "staticlib/compress/compress_exception.hpp"

namespace staticlib {
namespace compress {

/**
 * Sink wrapper that compressed written data using Deflate algorithm
 */
template <typename Sink, int compression_level = 6, std::size_t buf_size = 4096>
class deflate_sink {
    /**
     * Destination sink for the compressed data
     */
    Sink sink;
    /**
     * Internal buffer
     */
    std::array<char, buf_size> buf;
    /**
     * Zlib compressing stream
     */
    z_stream* strm;
    
public:
    
    /**
     * Constructor
     * 
     * @param sink destination to write compressed data into
     */
    deflate_sink(Sink&& sink) :
    sink(std::move(sink)),
    strm([] {
        z_stream* stream = static_cast<z_stream*> (std::malloc(sizeof(z_stream)));
        if (nullptr == stream) throw compress_exception(TRACEMSG(
                "Error creating deflate stream: 'malloc' failed"));
        std::memset(stream, 0, sizeof (z_stream));
        auto err = deflateInit2(stream, compression_level, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
        if (Z_OK != err) throw compress_exception(TRACEMSG(
                "Error initializing deflate stream: [" + ::zError(err) + "]"));
        return stream;
    }()) { }

    ~deflate_sink() STATICLIB_NOEXCEPT {
        if (nullptr == strm) return;
        auto deferred = sl::support::defer([this]() STATICLIB_NOEXCEPT {
            ::deflateEnd(this->strm);
            ::free(this->strm);
        });
        // finish encoding
        strm->next_in = nullptr;
        strm->avail_in = 0;
        strm->next_out = reinterpret_cast<unsigned char*> (buf.data());
        strm->avail_out = static_cast<uInt> (buf.size());
        // call deflate
        bool deflating = true;
        while(deflating) {
            auto err = ::deflate(strm, Z_FINISH);
            // cannot report any error safely - we are in destructor
            switch (err) {
            case Z_OK:
                if (strm->avail_out < buf.size()) {
                    sl::io::write_all(sink, {buf.data(), buf.size() - strm->avail_out});
                    strm->next_out = reinterpret_cast<unsigned char*> (buf.data());
                    strm->avail_out = static_cast<uInt> (buf.size());
                }
                // still not finished
                break;
            case Z_STREAM_END:
                if (strm->avail_out < buf.size()) {
                    sl::io::write_all(sink, {buf.data(), buf.size() - strm->avail_out});
                }
                // finished
                deflating = false;
                break;
            default:
                // finish
                deflating = false;
            }
        }
    }
    
    /**
     * Deleted copy constructor
     * 
     * @param other instance
     */
    deflate_sink(const deflate_sink&) = delete;

    /**
     * Deleted copy assignment operator
     * 
     * @param other instance
     * @return this instance 
     */
    deflate_sink& operator=(const deflate_sink&) = delete;

    /**
     * Move constructor
     * 
     * @param other other instance
     */
    deflate_sink(deflate_sink&& other) :
    sink(std::move(other.sink)),
    buf(std::move(other.buf)),
    strm(other.strm) {
        other.strm = nullptr;
    }

    /**
     * Move assignment operator
     * 
     * @param other other instance
     * @return this instance
     */
    deflate_sink& operator=(deflate_sink&& other) {
        sink = std::move(other.sink);
        buf = std::move(other.buf);
        strm = other.strm;
        other.strm = nullptr;
        return *this;
    }

    /**
     * Write implementation
     * 
     * @param span source span
     * @return number of bytes processed (read from source span)
     */
    std::streamsize write(sl::io::span<const char> span) {
        // prepare zlib stream
        strm->next_in = reinterpret_cast<const unsigned char*> (span.data());
        strm->avail_in = static_cast<uInt> (span.size());
        strm->next_out = reinterpret_cast<unsigned char*> (buf.data());
        strm->avail_out = static_cast<uInt> (buf.size());
        // call deflate
        while(strm->avail_in > 0) {
            auto err = ::deflate(strm, Z_NO_FLUSH);
            switch (err) {
            case Z_OK:
                if (strm->avail_out < buf.size()) {
                    sl::io::write_all(sink, {buf.data(), buf.size() - strm->avail_out});
                    strm->next_out = reinterpret_cast<unsigned char*> (buf.data());
                    strm->avail_out = static_cast<uInt> (buf.size());
                }
                break;
            default: throw compress_exception(TRACEMSG(
                        "Deflate error: [" + ::zError(err) + "]"));
            }
        }
        return span.size_signed();
    }

    /**
     * Calls flush on dest stream
     * 
     * @return value returned by dest stream
     */
    std::streamsize flush() {
        // note: maybe it is better to enable Z_SYNC_FLUSH support,
        // but it will make output result non-deterministic
        return sink.flush();
    }

    /**
     * Underlying sink accessor
     * 
     * @return underlying sink reference
     */
    Sink& get_sink() {
        return sink;
    }

};

/**
 * Factory function for creating deflate sinks,
 * created object will own the specified sink
 * 
 * @param sink output sink
 * @return deflate sink
 */
template <typename Sink,
        class = typename std::enable_if<!std::is_lvalue_reference<Sink>::value>::type>
deflate_sink<Sink> make_deflate_sink(Sink&& sink) {
    return deflate_sink<Sink>(std::move(sink));
}

/**
 * Factory function for creating deflate sinks,
 * created object will NOT own the specified sink
 * 
 * @param sink output sink
 * @return deflate sink
 */
template <typename Sink>
deflate_sink<sl::io::reference_sink<Sink>> make_deflate_sink(Sink& sink) {
    return deflate_sink<sl::io::reference_sink<Sink>> (
            sl::io::make_reference_sink(sink));
}

} // namespace
}

#endif /* STATICLIB_COMPRESS_DEFLATE_SINK_HPP */

