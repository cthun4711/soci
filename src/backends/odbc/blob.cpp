//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci-odbc.h"
#include <algorithm>

using namespace soci;
using namespace soci::details;


odbc_blob_backend::odbc_blob_backend(odbc_session_backend &session)
    : session_(session)
    , chunksize_(0)
    , srcdata_(nullptr)
    , srcsize_(0)
{
    // ...
}

odbc_blob_backend::~odbc_blob_backend()
{
    // ...
}

std::size_t odbc_blob_backend::get_len()
{
    // ...
    return 0;
}

std::size_t odbc_blob_backend::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    // ...
    return 0;
}

std::size_t odbc_blob_backend::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    // ...
    return 0;
}

std::size_t odbc_blob_backend::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    // ...
    return 0;
}

void odbc_blob_backend::trim(std::size_t /* newLen */)
{
    // ...
}

std::unique_ptr<std::string> odbc_blob_backend::read()
{
    std::unique_ptr<std::string> retObj = std::unique_ptr<std::string>(new std::string());

    SQLHSTMT hstmt = statement_->hstmt_;
    SQLSMALLINT colnum = static_cast<SQLSMALLINT>(position_);
    SQLINTEGER ind = 0;

    char dummy[1];
    SQLRETURN rc = SQLGetData( hstmt, colnum, SQL_C_BINARY, dummy, 0, &ind);
    if (rc == SQL_SUCCESS_WITH_INFO)
    {
        const SQLINTEGER DATASIZE = ind;
        retObj->resize(DATASIZE);

        size_t curr = 0;
        while( ((rc = SQLGetData( hstmt, colnum, SQL_C_BINARY, &(*retObj)[curr], DATASIZE, &ind)) == SQL_SUCCESS)
                || rc == SQL_SUCCESS_WITH_INFO )
        {
            const size_t bytes = std::min<SQLINTEGER>(DATASIZE, ind);
            curr += bytes;

            if( rc == SQL_SUCCESS )
                break;
        }
    }

    if( is_odbc_error(rc) )
    {
        retObj->clear();
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt, "reading BLOB");
    }

    return retObj;
}

void odbc_blob_backend::set_data_source(
    const char* src, const size_t& srcsz)
{
    srcdata_= src;
    srcsize_ =srcsz;
}

void odbc_blob_backend::upload()
{
    if( srcdata_ && srcsize_ > 0 )
    {
        SQLHSTMT hstmt = statement_->hstmt_;
        const SQLLEN CHUNKSIZE = chunksize_ > 0 ? chunksize_ : srcsize_;
        const char* endp = srcdata_ + srcsize_;
        for(const char* p=srcdata_ ; p<endp ; p+=CHUNKSIZE)
        {
            SQLRETURN rc = SQLPutData( hstmt, (SQLPOINTER)p, CHUNKSIZE);
            if( is_odbc_error(rc) )
            {
                throw odbc_soci_error( SQL_HANDLE_STMT, hstmt, "uploading BLOB");
            }
        }
    }
}
