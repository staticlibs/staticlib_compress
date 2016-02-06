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
#define	STATICLIB_COMPRESS_LZMA_SINK_HPP

#include <ios>
#include <memory>
#include <type_traits>
#include <cstdint>

#include "lzma.h"

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"

#include "staticlib/compress/CompressException.hpp"

namespace staticlib {
namespace compress {

namespace detail {

template <typename Sink>
class LzmaDeleter {
    Sink& sink;
    char* buf;
    size_t buf_size;
public:

    LzmaDeleter(Sink& sink, char* buf, size_t buf_size) :
    sink(sink),
    buf(buf),
    buf_size(buf_size) { }

    void operator()(lzma_stream* strm) {
        // finish encoding
        strm->next_in = nullptr;
        strm->avail_in = 0;
        strm->next_out = reinterpret_cast<uint8_t*>(buf);
        strm->avail_out = buf_size;
        // call deflate
        for (;;) {
            auto err = ::lzma_code(strm, LZMA_FINISH);
            switch (err) {
            case LZMA_OK:
                staticlib::io::write_all(sink, buf, buf_size - strm->avail_out);
                strm->next_out = reinterpret_cast<uint8_t*>(buf);
                strm->avail_out = buf_size;
                break;
            case LZMA_STREAM_END:
                staticlib::io::write_all(sink, buf, buf_size - strm->avail_out);
                // fall through
            default:
                // cannot report any error safely - we are in destructor
                goto end;
            }
        }
    end:
        ::lzma_end(strm);
        delete strm;
    }
};

} // namespace

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
     * Zlib compressing stream
     */
    std::unique_ptr<lzma_stream, detail::LzmaDeleter<Sink>> strm;

public:

    /**
     * Constructor
     * 
     * @param sink destination to write compressed data into
     */
    lzma_sink(Sink&& sink) :
    sink(std::move(sink)),
    strm(create_stream()) { }

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
    strm(std::move(other.strm)) { }

    /**
     * Move assignment operator
     * 
     * @param other other instance
     * @return this instance
     */
    lzma_sink& operator=(lzma_sink&& other) {
        sink = std::move(other.sink);
        buf = std::move(other.buf);
        strm = std::move(other.strm);
        return *this;
    }

    /**
     * Write implementation
     * 
     * @param b source buffer
     * @param length number of bytes to process
     * @return number of bytes processed (read from source buf)
     */
    std::streamsize write(const char* buffer, std::streamsize len_in) {
        // prepare lzma stream
        strm->next_in = reinterpret_cast<const uint8_t*>(buffer);
        strm->avail_in = static_cast<size_t> (len_in);
        strm->next_out = reinterpret_cast<uint8_t*>(buf.data());
        strm->avail_out = buf.size();
        // call code
        while (strm->avail_in > 0) {
            auto err = ::lzma_code(strm.get(), LZMA_RUN);
            switch (err) {
            case LZMA_OK:
                staticlib::io::write_all(sink, buf.data(), buf.size() - strm->avail_out);
                strm->next_out = reinterpret_cast<uint8_t*>(buf.data());
                strm->avail_out = buf.size();
                break;
            default: throw CompressException(TRACEMSG(std::string() +
                        "LZMA error code: [" + staticlib::config::to_string(err) + "]"));
            }
        }
        return len_in;
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

private:

    std::unique_ptr<lzma_stream, detail::LzmaDeleter<Sink>> create_stream() {
        std::unique_ptr<lzma_stream, detail::LzmaDeleter<Sink>>strm {
            new lzma_stream, detail::LzmaDeleter<Sink>(sink, buf.data(), buf.size())};
        *strm = LZMA_STREAM_INIT;
        auto err = lzma_easy_encoder(strm.get(), compression_level, LZMA_CHECK_CRC64);
        if (LZMA_OK != err) throw CompressException(TRACEMSG(std::string() +
                "Error initializing LZMA stream, code: [" + staticlib::config::to_string(err) + "]"));
        return strm;
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
lzma_sink<Sink> make_lzma_sink(Sink&& sink) {
    return lzma_sink<Sink>(std::move(sink));
}

/**
 * Factory function for creating deflate sinks,
 * created object will NOT own the specified sink
 * 
 * @param sink output sink
 * @return deflate sink
 */
template <typename Sink>
lzma_sink<staticlib::io::reference_sink<Sink>> make_lzma_sink(Sink& sink) {
    return lzma_sink<staticlib::io::reference_sink<Sink>> (
            staticlib::io::make_reference_sink(sink));
}

} // namespace
}


#endif	/* STATICLIB_COMPRESS_LZMA_SINK_HPP */

