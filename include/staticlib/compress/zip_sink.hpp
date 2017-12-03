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
#define STATICLIB_COMPRESS_ZIP_SINK_HPP

#include <ios>
#include <memory>
#include <string>
#include <vector>

#include "zlib.h"

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/endian.hpp"

#include "staticlib/compress/compress_exception.hpp"
#include "staticlib/compress/deflate_sink.hpp"
#include "staticlib/compress/zip_compression_method.hpp"

namespace staticlib {
namespace compress {

namespace detail {

/**
 * https://en.wikipedia.org/wiki/Zip_%28file_format%29
 */   
class Header {
    std::string filename;
    uint16_t compression_method;
    
    uint32_t offset = 0;
    uint32_t compressed_size = 0;
    uint32_t uncompressed_size = 0;
    uint32_t crc = 0;
    
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
        io::write_all(sink, {filename.data(), filename.length()});
        // save offset for CD
        this->offset = offset;
    }

    template <typename Sink>
    void write_data_descriptor(Sink& sink, uint32_t compressed_size, uint32_t uncompressed_size, uint32_t crc) {
        namespace en = staticlib::endian;
        // Optional data descriptor signature 
        en::write_32_le(sink, 0x08074b50);
        // CRC-32
        en::write_32_le(sink, crc);
        // Compressed size
        en::write_32_le(sink, compressed_size);
        // Uncompressed size
        en::write_32_le(sink, uncompressed_size);
        // save sizes for CD
        this->compressed_size = compressed_size;
        this->uncompressed_size = uncompressed_size;
        // save crc for CD
        this->crc = crc;
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
        en::write_32_le(sink, crc);
        // Compressed size
        en::write_32_le(sink, compressed_size);
        // Uncompressed size
        en::write_32_le(sink, uncompressed_size);
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
        io::write_all(sink, {filename.data(), filename.length()});
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
 * CRC sum and compression of entries is NOT supported
 */
template <typename Sink>
class zip_sink {
private:
    // stream shortcuts
    using sink_ref_type = sl::io::reference_sink<sl::io::counting_sink<Sink>>;
    using entry_counter_ref_type = sl::io::reference_sink<sl::io::counting_sink<sink_ref_type>>;
    using deflater_type = sl::io::counting_sink<deflate_sink<entry_counter_ref_type>>;

    /**
     * Destination sink for the zipped data
     */
    zip_compression_method method = zip_compression_method::deflate;
    sl::io::counting_sink<Sink> sink;
    std::vector<detail::Header> headers;
    bool cd_written = false;

    sl::io::counting_sink<sink_ref_type> entry_counter;
    std::unique_ptr<deflater_type> entry_deflater;
    uint32_t entry_crc = 0;

public:

    /**
     * Constructor
     * 
     * @param sink destination to write compressed data into
     */
    zip_sink(Sink&& sink) :
    sink(sl::io::make_counting_sink(std::move(sink))),
    entry_counter(sl::io::make_counting_sink(this->sink)),
    entry_deflater(nullptr) { }

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
    cd_written(other.cd_written),
    entry_counter(std::move(other.entry_counter)),
    entry_deflater(std::move(other.entry_deflater)),
    entry_crc(other.entry_crc) { }

    /**
     * Move assignment operator
     * 
     * @param other other instance
     * @return this instance
     */
    zip_sink& operator=(zip_sink&& other) {
        sink = std::move(other.sink);
        headers = std::move(other.headers);
        cd_written = other.cd_written;
        entry_counter = std::move(other.entry_counter);
        entry_deflater = std::move(other.entry_deflater);
        entry_crc = other.entry_crc;
        return *this;
    }

    /**
     * Write implementation
     * 
     * @param span source span
     * @return number of bytes processed (read from source buf)
     */
    std::streamsize write(sl::io::span<const char> span) {
        if (nullptr != entry_deflater.get()) {
            size_t count_before = entry_deflater->get_count();
            entry_deflater->write(span);
            size_t count = entry_deflater->get_count() - count_before;
            this->entry_crc = ::crc32(entry_crc, reinterpret_cast<const Bytef*>(span.data()), count);
            return static_cast<std::streamsize>(count);
        } else {
            throw compress_exception(TRACEMSG("Invalid ZIP sink state: add ZIP entry before writing the data"));
        }
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
    void add_entry(const std::string& filename) {
        if (filename.empty()) throw compress_exception(TRACEMSG("Invalid empty entry name specified"));
        if (cd_written) throw new compress_exception(TRACEMSG( 
                "Invalid entry add attempt for finalized ZIP stream"));
        if (headers.size() > 0) {
            write_entry_data_descriptor();
        }
        // add new entry
        headers.emplace_back(std::string(filename.data(), filename.length()), static_cast<uint16_t>(method));
        headers.back().write_local_file_header(sink, static_cast<uint32_t>(sink.get_count()));
        entry_deflater.reset(new sl::io::counting_sink<deflate_sink<entry_counter_ref_type>>(make_deflate_sink(entry_counter)));
        entry_crc = ::crc32(0L, Z_NULL, 0);
    }
    
    /**
     * Finalizes ZIP archive writing Central Directory,
     * may be safely called multiple times,
     * will be called from destructor if not called explicitely
     */
    void finalize() {
        if (!cd_written && headers.size() > 0) {
            write_entry_data_descriptor();
            uint32_t cd_offset = static_cast<uint32_t>(sink.get_count());
            for (detail::Header& he : headers) {
                he.write_cd_file_header(sink);
            }
            size_t cd_len = sink.get_count() - cd_offset;
            detail::write_eocd(sink, static_cast<uint16_t>(headers.size()), cd_offset, static_cast<uint32_t>(cd_len));
            cd_written = true;
        }
    }

private:
    void write_entry_data_descriptor() {
        // get size and close current entry
        uint32_t uncompressed_size = static_cast<uint32_t>(entry_deflater->get_count());
        entry_deflater.reset(nullptr);
        // get size and reset counter
        uint32_t compressed_size = static_cast<uint32_t>(entry_counter.get_count());
        entry_counter = sl::io::make_counting_sink(sink);
        headers.back().write_data_descriptor(sink, compressed_size, uncompressed_size, entry_crc);
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
zip_sink<sl::io::reference_sink<Sink>> make_zip_sink(Sink& sink) {
    return zip_sink<sl::io::reference_sink<Sink>> (
            sl::io::make_reference_sink(sink));
}

} // namespace
}

#endif /* STATICLIB_COMPRESS_ZIP_SINK_HPP */

