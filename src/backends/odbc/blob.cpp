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

std::unique_ptr<std::string> odbc_blob_backend::read(mn_odbc_error_info& err_info)
{
    std::unique_ptr<std::string> retObj = std::unique_ptr<std::string>(new std::string());

    SQLHSTMT hstmt = statement_->hstmt_;
    SQLSMALLINT colnum = static_cast<SQLSMALLINT>(position_);
    SQLLEN ind = 0;

    const SQLLEN BUFSIZE = 32*1024;
    char* buf = new char[BUFSIZE];
    if( buf != nullptr )
    {
        SQLRETURN rc;
        while (((rc = SQLGetData(hstmt, colnum, SQL_C_BINARY, buf, BUFSIZE, &ind)) == SQL_SUCCESS)
            || rc == SQL_SUCCESS_WITH_INFO)
        {
            const size_t bytes = std::min<SQLINTEGER>(BUFSIZE, ind);
            retObj->append(buf, bytes);

            if (rc == SQL_SUCCESS)
                break;
        }

        delete buf;
        buf = nullptr;

        if( is_odbc_error(rc) )
        {
            retObj->clear();
            odbc_soci_error myErr( SQL_HANDLE_STMT, hstmt, "reading BLOB");

            err_info.native_error_code_ = myErr.native_error_code();
            err_info.odbc_error_message_ = (char*)myErr.odbc_error_message();
            err_info.odbc_func_name_ = "SQLGetData";
            err_info.odbc_func_returnval_ = rc;
        }
    }
    else
    {
            err_info.native_error_code_ = SQL_ERROR;
            err_info.odbc_error_message_ = "failed to allocate buffer";
            err_info.odbc_func_name_ = "";
            err_info.odbc_func_returnval_ = SQL_ERROR;
    }

    return retObj;
}

void odbc_blob_backend::set_data_source(
    const char* src, const size_t& srcsz)
{
    srcdata_= src;
    srcsize_ =srcsz;
}

SQLRETURN odbc_blob_backend::upload(mn_odbc_error_info& err_info)
{
    SQLRETURN rc = SQL_SUCCESS;
    if( srcdata_ && srcsize_ > 0 )
    {
        SQLHSTMT hstmt = statement_->hstmt_;
        const SQLLEN CHUNKSIZE = chunksize_ > 0 ? chunksize_ : srcsize_;
        const char* endp = srcdata_ + srcsize_;
        for(const char* p=srcdata_ ; p<endp ; p+=CHUNKSIZE)
        {
            rc = SQLPutData( hstmt, (SQLPOINTER)p, CHUNKSIZE);
            if( is_odbc_error(rc) )
            {
                odbc_soci_error myErr(SQL_HANDLE_STMT, hstmt, "uploading BLOB");

                err_info.native_error_code_ = myErr.native_error_code();
                err_info.odbc_error_message_ = (char*)myErr.odbc_error_message();
                err_info.odbc_func_name_ = "SQLPutData";
                err_info.odbc_func_returnval_ = rc;
                break;
            }
        }
    }

    return rc;
}
