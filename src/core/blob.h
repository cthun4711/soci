//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BLOB_H_INCLUDED
#define SOCI_BLOB_H_INCLUDED

#include "soci-config.h"
#include "error.h"
// std
#include <cstddef>
#include <string>
#include <memory>

namespace soci
{
// basic blob operations

class session;

namespace details
{
class blob_backend;
} // namespace details

class SOCI_DECL blob
{
public:
    explicit blob(session & s);
    ~blob();

    std::size_t get_len();
    std::size_t read(std::size_t offset, char * buf, std::size_t toRead);
    std::size_t write(std::size_t offset, char const * buf,
        std::size_t toWrite);
    std::size_t append(char const * buf, std::size_t toWrite);
    void trim(std::size_t newLen);

    // TBD return std::unique_ptr<std::vector<char>>
    // or even std::pair<std::unique_ptr<char>, size_t>
    // instead of std::unique_ptr<std::string>?
#if _MSC_VER > 1900
    std::unique_ptr<std::string> read(mn_odbc_error_info& err_info);
#endif
    void set_data_source(const char* src, const size_t& srcsz);
    void set_data_source(const std::string& src);

    details::blob_backend * get_backend() { return backEnd_; }

private:
    details::blob_backend * backEnd_;
};

} // namespace soci

#endif
