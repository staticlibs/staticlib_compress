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
 * File:   zip_sink.hpp
 * Author: alex
 *
 * Created on February 13, 2016, 5:30 PM
 */

#ifndef STATICLIB_COMPRESS_ZIP_SINK_HPP
#define	STATICLIB_COMPRESS_ZIP_SINK_HPP

#include <ios>
#include <string>

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/endian.hpp"
#include "staticlib/compress/CompressException.hpp"

namespace staticlib {
namespace compress {

namespace detail {

/**
 * https://en.wikipedia.org/wiki/Zip_%28file_format%29
 */   
class Header {
    std::string filename;
    // todo: compressed/uncompressed sizes support, currently they are the same
    uint16_t compression_method;
    
    uint32_t offset = 0;
    uint32_t length = 0;
    
public:
    
    Header(std::string filename, uint16_t compression_method) :
    filename(std::move(filename)),
    compression_method(compression_method) { }

    template <typename Sink>
    void write_local_file_header(Sink& sink, uint32_t offset) {
        namespace en = staticlib::endian;
        // Local file header signature
        en::write_32_le(sink, 0x04034b50);
        // Version needed to extract (minimum)
        en::write_16_le(sink, 10);
        // General purpose bit flag
        en::write_16_le(sink, 8);
        // Compression method
        en::write_16_le(sink, compression_method);
        // File last modification time
        en::write_16_le(sink, 0);
        // File last modification date
        en::write_16_le(sink, 0);
        // CRC-32
        en::write_32_le(sink, 0);
        // Compressed size
        en::write_32_le(sink, 0);
        // Uncompressed size
        en::write_32_le(sink, 0);
        // File name length (n)
        en::write_16_le(sink, filename.length());
        // Extra field length (m)
        en::write_16_le(sink, 0);
        // File name
        io::write_all(sink, filename.data(), filename.length());
        // save offset for CD
        this->offset = offset;
    }

    template <typename Sink>
    void write_data_descriptor(Sink& sink, uint32_t length) {
        namespace en = staticlib::endian;
        // Optional data descriptor signature 
        en::write_32_le(sink, 0x08074b50);
        // CRC-32
        en::write_32_le(sink, 0);
        // Compressed size
        en::write_32_le(sink, length);
        // Uncompressed size
        en::write_32_le(sink, length);
        // save size for CD
        this->length = length;
    }

    template <typename Sink>
    void write_cd_file_header(Sink& sink) { 
        namespace en = staticlib::endian;
        // Central directory file header signature
        en::write_32_le(sink, 0x02014b50);
        // Version made by
        en::write_16_le(sink, 10);
        // Version needed to extract (minimum)
        en::write_16_le(sink, 10);
        // General purpose bit flag
        en::write_16_le(sink, 8);
        // Compression method
        en::write_16_le(sink, compression_method);
        // File last modification time
        en::write_16_le(sink, 0);
        // File last modification date
        en::write_16_le(sink, 0);
        // CRC-32
        en::write_32_le(sink, 0);
        // Compressed size
        en::write_32_le(sink, length);
        // Uncompressed size
        en::write_32_le(sink, length);
        // File name length (n)
        en::write_16_le(sink, filename.length());
        // Extra field length (m)
        en::write_16_le(sink, 0);
        // File comment length (k)
        en::write_16_le(sink, 0);
        // Disk number where file starts
        en::write_16_le(sink, 0);
        // Internal file attributes
        en::write_16_le(sink, 0);
        // External file attributes
        en::write_32_le(sink, 0);
        // Relative offset of local file header.
        en::write_32_le(sink, offset);
        // File name
        io::write_all(sink, filename.data(), filename.length());
    }
    
};

template <typename Sink>
void write_eocd(Sink& sink, uint16_t files_count, uint32_t cd_offset,  uint32_t cd_length) {
    namespace en = staticlib::endian;
    // End of central directory signature
    en::write_32_le(sink, 0x06054b50);
    // Number of this disk
    en::write_16_le(sink, 0);
    // Disk where central directory starts
    en::write_16_le(sink, 0);
    // Number of central directory records on this disk
    en::write_16_le(sink, files_count);
    // Total number of central directory records
    en::write_16_le(sink, files_count);
    // Size of central directory (bytes)
    en::write_32_le(sink, cd_length);
    // Offset of start of central directory
    en::write_32_le(sink, cd_offset);
    // Comment length (n)
    en::write_16_le(sink, 0);
}

} // namespace

/**
 * Sink wrapper that creates ZIP archives,
 * compression of entries is NOT supported
 */
template <typename Sink>
class zip_sink {
    /**
     * Destination sink for the zipped data
     */
    staticlib::io::counting_sink<Sink> sink;
    std::vector<detail::Header> headers;
    std::streamsize entry_start = 0;
    bool cd_written = false;

public:

    /**
     * Constructor
     * 
     * @param sink destination to write compressed data into
     */
    zip_sink(Sink&& sink) :
    sink(staticlib::io::make_counting_sink(std::move(sink))) { }

    /**
     * Destructor, will call `finalize()` if it have not been called yet
     */
    ~zip_sink() STATICLIB_NOEXCEPT {
        try {
            finalize();
        } catch(...) {
            // ignore
        }
    }
    
    /**
     * Deleted copy constructor
     * 
     * @param other instance
     */
    zip_sink(const zip_sink&) = delete;

    /**
     * Deleted copy assignment operator
     * 
     * @param other instance
     * @return this instance 
     */
    zip_sink& operator=(const zip_sink&) = delete;

    /**
     * Move constructor
     * 
     * @param other other instance
     */
    zip_sink(zip_sink&& other) :
    sink(std::move(other.sink)),
    headers(std::move(other.headers)),
    entry_start(other.entry_start),
    cd_written(other.cd_written) { }

    /**
     * Move assignment operator
     * 
     * @param other other instance
     * @return this instance
     */
    zip_sink& operator=(zip_sink&& other) {
        sink = std::move(other.sink);
        headers = std::move(other.headers);
        entry_start = other.entry_start;
        cd_written = other.cd_written;
        return *this;
    }

    /**
     * Write implementation
     * 
     * @param b source buffer
     * @param length number of bytes to process
     * @return number of bytes processed (read from source buf)
     */
    std::streamsize write(const char* buffer, std::streamsize len) {
        return sink.write(buffer, len);
    }

    /**
     * Calls flush on dest stream
     * 
     * @return value returned by dest stream
     */
    std::streamsize flush() {
        return sink.flush();
    }
    
    /**
     * Add ZIP entry with the specified name to archive
     * 
     * @param filename ZIP entry name
     */
    void add_entry(std::string filename) {
        if (cd_written) throw new CompressException(TRACEMSG(std::string() + 
                "Invalid entry add attempt for finalized ZIP stream"));
        if (headers.size() > 0) {
            headers.back().write_data_descriptor(sink, sink.get_count() - entry_start);
        }
        // currently no support for compression
        headers.emplace_back(std::move(filename), 0);
        headers.back().write_local_file_header(sink, sink.get_count());
        entry_start = sink.get_count();
    }
    
    /**
     * Finalizes ZIP archive writing Central Directory,
     * may be safely called multiple times,
     * will be called from destructor if not called explicitely
     */
    void finalize() {
        if (!cd_written && headers.size() > 0) {
            headers.back().write_data_descriptor(sink, sink.get_count() - entry_start);
            uint32_t cd_offset = sink.get_count();
            for (detail::Header& he : headers) {
                he.write_cd_file_header(sink);
            }
            detail::write_eocd(sink, headers.size(), cd_offset, sink.get_count() - cd_offset);
            cd_written = true;
        }
    }
};

/**
 * Factory function for creating zip sinks,
 * created object will own the specified sink
 * 
 * @param sink output sink
 * @return zip sink
 */
template <typename Sink,
class = typename std::enable_if<!std::is_lvalue_reference<Sink>::value>::type>
zip_sink<Sink> make_zip_sink(Sink&& sink) {
    return zip_sink<Sink>(std::move(sink));
}

/**
 * Factory function for creating zip sinks,
 * created object will NOT own the specified sink
 * 
 * @param sink output sink
 * @return zip sink
 */
template <typename Sink>
zip_sink<staticlib::io::reference_sink<Sink>> make_zip_sink(Sink& sink) {
    return zip_sink<staticlib::io::reference_sink<Sink>> (
            staticlib::io::make_reference_sink(sink));
}

} // namespace
}

#endif	/* STATICLIB_COMPRESS_ZIP_SINK_HPP */

