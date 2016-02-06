/* 
 * File:   CompressException.hpp
 * Author: alex
 *
 * Created on January 29, 2016, 10:19 PM
 */

#ifndef STATICLIB_COMPRESS_COMPRESSEXCEPTION_HPP
#define	STATICLIB_COMPRESS_COMPRESSEXCEPTION_HPP

#include "staticlib/config/BaseException.hpp"

namespace staticlib {
namespace compress {

/**
 * Exception that is thrown on compression/decompression error
 */
class CompressException : public staticlib::config::BaseException {
public:
    /**
     * Default constructor
     */
    CompressException() = default;

    /**
     * Constructor with message
     * 
     * @param msg error message
     */
    CompressException(const std::string& msg) :
    staticlib::config::BaseException(msg) { }

};

} // namespace
}

#endif	/* STATICLIB_COMPRESS_COMPRESSEXCEPTION_HPP */

