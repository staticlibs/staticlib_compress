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
 * File:   lzma_sink.hpp
 * Author: alex
 *
 * Created on January 29, 2016, 9:21 PM
 */

#ifndef STATICLIB_COMPRESS_LZMA_SINK_HPP
#define STATICLIB_COMPRESS_LZMA_SINK_HPP

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <memory>
#include <type_traits>

#include "lzma.h"

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/support.hpp"

#include "staticlib/compress/compress_exception.hpp"

namespace staticlib {
namespace compress {

/**
 * Sink wrapper that compressed written data using LZMA algorithm
 */
template <typename Sink, int compression_level = 6, std::size_t buf_size = 4096 >
class lzma_sink {
    /**
     * Destination sink for the compressed data
     */
    Sink sink;
    /**
     * Internal buffer
     */
    std::array<char, buf_size> buf;
    /**
     * LZMA compressing stream
     */
    lzma_stream* strm;

public:

    /**
     * Constructor
     * 
     * @param sink destination to write compressed data into
     */
    lzma_sink(Sink&& sink) :
    sink(std::move(sink)),
    strm([] {
        lzma_stream* strm = static_cast<lzma_stream*> (std::malloc(sizeof(lzma_stream)));
        if (nullptr == strm) throw compress_exception(TRACEMSG(
                "Error creating lzma stream: 'malloc' failed"));
        *strm = LZMA_STREAM_INIT;
        auto err = ::lzma_easy_encoder(strm, compression_level, LZMA_CHECK_CRC64);
        if (LZMA_OK != err) throw compress_exception(TRACEMSG(
                "Error initializing LZMA stream, code: [" + sl::support::to_string(err) + "]"));
        return strm;
    }()) { }

    ~lzma_sink() STATICLIB_NOEXCEPT {
        if (nullptr == strm) return;
        auto deferred = sl::support::defer([this]() STATICLIB_NOEXCEPT {
            ::lzma_end(this->strm);
            ::free(this->strm);
        });
        // finish encoding
        strm->next_in = nullptr;
        strm->avail_in = 0;
        strm->next_out = reinterpret_cast<uint8_t*>(buf.data());
        strm->avail_out = buf.size();
        // call code
        bool coding = true;
        while (coding) {
            auto err = ::lzma_code(strm, LZMA_FINISH);
            switch (err) {
            case LZMA_OK:
                sl::io::write_all(sink, {buf.data(), buf.size() - strm->avail_out});
                strm->next_out = reinterpret_cast<uint8_t*>(buf.data());
                strm->avail_out = buf.size();
                // not finished
                break;
            case LZMA_STREAM_END:
                sl::io::write_all(sink, {buf.data(), buf.size() - strm->avail_out});
                // finished
                coding = false;
                break;
            default:
                // finish
                coding = false;
            }
        }
    }

    /**
     * Deleted copy constructor
     * 
     * @param other instance
     */
    lzma_sink(const lzma_sink&) = delete;

    /**
     * Deleted copy assignment operator
     * 
     * @param other instance
     * @return this instance 
     */
    lzma_sink& operator=(const lzma_sink&) = delete;

    /**
     * Move constructor
     * 
     * @param other other instance
     */
    lzma_sink(lzma_sink&& other) :
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
    lzma_sink& operator=(lzma_sink&& other) {
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
        // prepare lzma stream
        strm->next_in = reinterpret_cast<const uint8_t*>(span.data());
        strm->avail_in = static_cast<size_t> (span.size());
        strm->next_out = reinterpret_cast<uint8_t*>(buf.data());
        strm->avail_out = buf.size();
        // call code
        while (strm->avail_in > 0) {
            auto err = ::lzma_code(strm, LZMA_RUN);
            switch (err) {
            case LZMA_OK:
                sl::io::write_all(sink, {buf.data(), buf.size() - strm->avail_out});
                strm->next_out = reinterpret_cast<uint8_t*>(buf.data());
                strm->avail_out = buf.size();
                break;
            default: throw compress_exception(TRACEMSG(
                        "LZMA error code: [" + sl::support::to_string(err) + "]"));
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
        // note: maybe it is better to enable LZMA_SYNC_FLUSH support,
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
 * Factory function for creating lzma sinks,
 * created object will own the specified sink
 * 
 * @param sink output sink
 * @return lzma sink
 */
template <typename Sink,
        class = typename std::enable_if<!std::is_lvalue_reference<Sink>::value>::type>
lzma_sink<Sink> make_lzma_sink(Sink&& sink) {
    return lzma_sink<Sink>(std::move(sink));
}

/**
 * Factory function for creating lzma sinks,
 * created object will NOT own the specified sink
 * 
 * @param sink output sink
 * @return lzma sink
 */
template <typename Sink>
lzma_sink<sl::io::reference_sink<Sink>> make_lzma_sink(Sink& sink) {
    return lzma_sink<sl::io::reference_sink<Sink>> (
            sl::io::make_reference_sink(sink));
}

} // namespace
}


#endif /* STATICLIB_COMPRESS_LZMA_SINK_HPP */

