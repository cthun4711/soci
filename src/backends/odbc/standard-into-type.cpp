//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include <soci-platform.h>
#include "soci-odbc.h"
#include "mnsocistring.h"
#include "blob.h"
#include <ctime>
#include <stdio.h>  // sscanf()

using namespace soci;
using namespace soci::details;

void odbc_standard_into_type_backend::define_by_pos(
    int & position, void * data, exchange_type type, SQLLEN* ind)
{
    data_ = data;
    type_ = type;
    position_ = position++;

    SQLUINTEGER size = 0;

    this->ind_ = ind;

    switch (type_)
    {
    case x_char:
        odbcType_ = SQL_C_CHAR;
        size = sizeof(char) + 1;
        buf_ = new char[size];
        data = buf_;
        break;
    case x_mnsocistring:
        odbcType_ = SQL_C_CHAR;
		size = ((MNSociString*)data_)->getSize();
        buf_ = &(((MNSociString*)data_)->m_ptrCharData[0]); //use the char* inside the odbc call!!
        data = buf_;
        this->ind_ = &((MNSociString*)data_)->m_iIndicator;
        break;
	case x_mnsocitext:
		odbcType_ = SQL_C_CHAR;
		size = ((MNSociText*)data_)->getSize();
		buf_ = &(((MNSociText*)data_)->m_ptrCharData[0]); //use the char* inside the odbc call!!
		data = buf_;
		this->ind_ = &((MNSociText*)data_)->m_iIndicator;
		break;
    case x_stdstring:
        odbcType_ = SQL_C_CHAR;
        // Patch: set to min between column size and 100MB (used ot be 32769)
        // Column size for text data type can be too large for buffer allocation
        size = 4000; //be able to handle the text columns (4K alloc should be faster than asking the db server about the column size)
        //size = size > odbc_max_buffer_length ? odbc_max_buffer_length : size;
        size++;
        //size = 257;
        buf_ = new char[size];
        data = buf_;
        break;
    case x_short:
        odbcType_ = SQL_C_SSHORT;
        size = sizeof(short);
        break;
    case x_integer:
        odbcType_ = SQL_C_SLONG;
        size = sizeof(int);
        break;
    case x_long_long:
        //if (use_string_for_bigint())
        //{
        //  odbcType_ = SQL_C_CHAR;
        //  size = max_bigint_length;
        //  buf_ = new char[size];
        //  data = buf_;
        //}
        //else // Normal case, use ODBC support.
        //{
        odbcType_ = SQL_C_SLONG;
          size = sizeof(long long);
        //}
        break;
    case x_unsigned_long_long:
        //if (use_string_for_bigint())
        //{
        //  odbcType_ = SQL_C_CHAR;
        //  size = max_bigint_length;
        //  buf_ = new char[size];
        //  data = buf_;
        //}
        //else // Normal case, use ODBC support.
        //{
        odbcType_ = SQL_C_SLONG;
          size = sizeof(unsigned long long);
        //}
        break;
    case x_double:
        odbcType_ = SQL_C_DOUBLE;
        size = sizeof(double);
        break;
    //case x_stdtm:
    //    odbcType_ = SQL_C_TYPE_TIMESTAMP;
    //    size = sizeof(TIMESTAMP_STRUCT);
    //    buf_ = new char[size];
    //    data = buf_;
    //    break;
    case x_odbctimestamp:
        odbcType_ = SQL_C_TYPE_TIMESTAMP;
        size = sizeof(TIMESTAMP_STRUCT);
        break;
    case x_odbcnumericstruct:
        odbcType_ = SQL_C_NUMERIC;
        size = sizeof(SQL_NUMERIC_STRUCT);
        break;
    case x_rowid:
        odbcType_ = SQL_C_ULONG;
        size = sizeof(unsigned long);
        break;
    case x_blob:
    {
        blob *b = static_cast<blob *>(data);

        odbc_blob_backend *bbe
            = static_cast<odbc_blob_backend *>(b->get_backend());

        bbe->statement_ = &statement_;
        bbe->position_ = position_;

        odbcType_ = SQL_C_BINARY;
        size = 0;
        return; // can't be bound
    }
    default:
        throw soci_error("Into element used with non-supported type.");
    }

    SQLRETURN rc = SQLBindCol(statement_.hstmt_, static_cast<SQLUSMALLINT>(position_),
        static_cast<SQLUSMALLINT>(odbcType_), data, size, ind);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_,
                            "into type pre_fetch");
    }

    //if (type_ == x_odbcnumericstruct)
    //{
    //    rc = SQLSetDescField(statement_.hstmt_, static_cast<SQLUSMALLINT>(position_), SQL_DESC_TYPE, (VOID*)SQL_C_NUMERIC, 0);
    //    rc = SQLSetDescField(statement_.hstmt_, static_cast<SQLUSMALLINT>(position_), SQL_DESC_PRECISION, (VOID*)16, 0);
    //    rc = SQLSetDescField(statement_.hstmt_, static_cast<SQLUSMALLINT>(position_), SQL_DESC_SCALE, (VOID*)15, 0);
    //}
}

void odbc_standard_into_type_backend::pre_fetch()
{
    //...
}

void odbc_standard_into_type_backend::post_fetch(bool gotData, bool calledFromFetch)
{
    if (x_stdstring != type_)
    { // only std::string and std::tm need special handling
        return;
    }

    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to do anything (fetch() will return false)
        return;
    }

    if (gotData)
    {
        // first, deal with indicators
        if (SQL_NULL_DATA == *ind_)
        {
            return;
        }

        // only std::string and std::tm need special handling
        if (x_stdstring == type_)
        {
            std::string *s = static_cast<std::string *>(data_);
            s->assign(buf_);
        }
    }
}

void odbc_standard_into_type_backend::clean_up()
{
    if (buf_)
    {
        if (type_ != x_mnsocistring && type_ != x_mnsocitext)
        {
            delete[] buf_;
        }
        buf_ = 0;
    }
}
