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
#define	STATICLIB_COMPRESS_DEFLATE_SINK_HPP

#include <ios>
#include <memory>
#include <type_traits>

#include "zlib.h"

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"

#include "staticlib/compress/CompressException.hpp"

namespace staticlib {
namespace compress {

namespace detail {

template <typename Sink>
class DeflateDeleter {
    Sink& sink;
    char* buf;
    size_t buf_size;
public:
    DeflateDeleter(Sink& sink, char* buf, size_t buf_size) :
    sink(sink),
    buf(buf),
    buf_size(buf_size) { }
    
    void operator()(z_stream* strm) {
        // finish encoding
        strm->next_in = nullptr;
        strm->avail_in = 0;
        strm->next_out = reinterpret_cast<unsigned char*> (buf);
        strm->avail_out = static_cast<uInt> (buf_size);
        // call deflate
        for(;;) {
            auto err = ::deflate(strm, Z_FINISH);
            switch (err) {
            case Z_OK:
                staticlib::io::write_all(sink, buf, buf_size - strm->avail_out);
                strm->next_out = reinterpret_cast<unsigned char*> (buf);
                strm->avail_out = static_cast<uInt> (buf_size);
                break;
            case Z_STREAM_END:
                staticlib::io::write_all(sink, buf, buf_size - strm->avail_out);                
                // fall through
            default:
                // cannot report any error safely - we are in destructor
                goto end;
            }
        }
    end: 
        ::deflateEnd(strm);
        delete strm;
    }
};

} // namespace

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
    std::unique_ptr<z_stream, detail::DeflateDeleter<Sink>> strm;
    
public:
    
    /**
     * Constructor
     * 
     * @param sink destination to write compressed data into
     */
    deflate_sink(Sink&& sink) :
    sink(std::move(sink)),
    strm(create_stream()) { }
    
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
    strm(std::move(other.strm)) { }

    /**
     * Move assignment operator
     * 
     * @param other other instance
     * @return this instance
     */
    deflate_sink& operator=(deflate_sink&& other) {
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
        // prepare zlib stream
        strm->next_in = reinterpret_cast<const unsigned char*> (buffer);
        strm->avail_in = static_cast<uInt> (len_in);
        strm->next_out = reinterpret_cast<unsigned char*> (buf.data());
        strm->avail_out = static_cast<uInt> (buf.size());
        // call deflate
        while(strm->avail_in > 0) {
            auto err = ::deflate(strm.get(), Z_NO_FLUSH);
            switch (err) {
            case Z_OK:
                staticlib::io::write_all(sink, buf.data(), buf.size() - strm->avail_out);
                strm->next_out = reinterpret_cast<unsigned char*> (buf.data());
                strm->avail_out = static_cast<uInt> (buf.size());
                break;
            default: throw CompressException(TRACEMSG(std::string() +
                        "Deflate error: [" + ::zError(err) + "]"));
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
        // note: maybe it is better to enable Z_SYNC_FLUSH support,
        // but it will make output result non-deterministic
        return sink.flush();
    }
    
private:

    std::unique_ptr<z_stream, detail::DeflateDeleter<Sink>> create_stream() {
        std::unique_ptr<z_stream, detail::DeflateDeleter<Sink>> strm{
            new z_stream, detail::DeflateDeleter<Sink>(sink, buf.data(), buf.size())};
        std::memset(strm.get(), 0, sizeof (z_stream));
        auto err = deflateInit(strm.get(), compression_level);
        if (Z_OK != err) throw CompressException(TRACEMSG(std::string() +
                "Error initializing deflate stream: [" + ::zError(err) + "]"));
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
deflate_sink<staticlib::io::reference_sink<Sink>> make_deflate_sink(Sink& sink) {
    return deflate_sink<staticlib::io::reference_sink<Sink>> (
            staticlib::io::make_reference_sink(sink));
}

} // namespace
}

#endif	/* STATICLIB_COMPRESS_DEFLATE_SINK_HPP */

