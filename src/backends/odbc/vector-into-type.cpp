//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci-odbc.h"
#include <soci-platform.h>
#include "mnsocistring.h"
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <stdio.h>  // sscanf()

using namespace soci;
using namespace soci::details;

void odbc_vector_into_type_backend::define_by_pos(
    int &position, void *data, exchange_type type, SQLLEN* indHolders)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    SQLLEN size = 0;       // also dummy

    switch (type)
    {
    // simple cases
    case x_short:
        {
            odbcType_ = SQL_C_SSHORT;
            size = sizeof(short);
            std::vector<short> *vp = static_cast<std::vector<short> *>(data);
            std::vector<short> &v(*vp);
            data = &v[0];
        }
        break;
    case x_integer:
        {
            odbcType_ = SQL_C_SLONG;
            size = sizeof(SQLINTEGER);
            assert(sizeof(SQLINTEGER) == sizeof(int));
            std::vector<int> *vp = static_cast<std::vector<int> *>(data);
            std::vector<int> &v(*vp);
            data = &v[0];
        }
        break;
    case x_long_long:
        {
            std::vector<long long> *vp =
                static_cast<std::vector<long long> *>(data);
            std::vector<long long> &v(*vp);
            odbcType_ = SQL_C_SLONG;
            size = sizeof(long long);
            data = &v[0];
        }
        break;
    case x_unsigned_long_long:
        {
            std::vector<unsigned long long> *vp =
                static_cast<std::vector<unsigned long long> *>(data);
            std::vector<unsigned long long> &v(*vp);
            odbcType_ = SQL_C_SLONG;
            size = sizeof(unsigned long long);
            data = &v[0];
        }
        break;
    case x_double:
        {
            odbcType_ = SQL_C_DOUBLE;
            size = sizeof(double);
            std::vector<double> *vp = static_cast<std::vector<double> *>(data);
            std::vector<double> &v(*vp);
            data = &v[0];
        }
        break;

    // cases that require adjustments and buffer management

    case x_char:
        {
            odbcType_ = SQL_C_CHAR;

            std::vector<char> *v
                = static_cast<std::vector<char> *>(data);

            size = sizeof(char) * 2;
            std::size_t bufSize = size * v->size();

            colSize_ = size;

            buf_ = new char[bufSize];
            data = buf_;
        }
        break;
    case x_mnsocistring:
    {
        odbcType_ = SQL_C_CHAR;
        std::vector<MNSociString> *v
            = static_cast<std::vector<MNSociString> *>(data);
        colSize_ = MNSociString::MNSOCI_SIZE;
        std::size_t bufSize = colSize_ * v->size();
        buf_ = new char[bufSize];

        size = static_cast<SQLINTEGER>(colSize_);
        data = buf_;
        break;
    }
	case x_mnsocitext:
		{
			odbcType_ = SQL_C_CHAR;
			std::vector<MNSociText> *v
				= static_cast<std::vector<MNSociText> *>(data);
			colSize_ = 4002;
			std::size_t bufSize = colSize_ * v->size();
			buf_ = new char[bufSize];

			size = static_cast<SQLINTEGER>(colSize_);
			data = buf_;
			break;
		}
    case x_mnsociarraystring:
    {
        odbcType_ = SQL_C_CHAR;
        MNSociArrayString* v = static_cast<MNSociArrayString *>(data);
        size = v->getStringSize();
        data = v->getArrayCharData();
        indHolders = v->getArrayIndicators();
        break;
    }
	case x_mnsociarraytext:
		{
			odbcType_ = SQL_C_CHAR;
			MNSociArrayText* v = static_cast<MNSociArrayText *>(data);
			size = v->getStringSize();
			data = v->getArrayCharData();
			indHolders = v->getArrayIndicators();
			break;
		}
    case x_stdstring:
        {
            odbcType_ = SQL_C_CHAR;
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data);
            //colSize_ = statement_.column_size(position) + 1;
            colSize_ = 257;
            std::size_t bufSize = colSize_ * v->size();
            buf_ = new char[bufSize];

            size = static_cast<SQLINTEGER>(colSize_);
            data = buf_;
        }
        break;

    case x_odbctimestamp:
    {
        odbcType_ = SQL_C_TYPE_TIMESTAMP;
        size = sizeof(TIMESTAMP_STRUCT);
        std::vector<TIMESTAMP_STRUCT> *vp = static_cast<std::vector<TIMESTAMP_STRUCT> *>(data);
        std::vector<TIMESTAMP_STRUCT> &v(*vp);
        data = &v[0];
        break;
    }

    case x_statement: break; // not supported
    case x_rowid:     break; // not supported
    case x_blob:      break; // not supported
    }

    SQLRETURN rc 
        = SQLBindCol(statement_.hstmt_, static_cast<SQLUSMALLINT>(position++),
                odbcType_, static_cast<SQLPOINTER>(data), size, indHolders);

    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_,
                            "vector into type define by pos");
    }
}

void odbc_vector_into_type_backend::pre_fetch()
{
    // nothing to do for the supported types
}

void odbc_vector_into_type_backend::post_fetch(bool gotData)
{
    if (gotData)
    {
        if (type_ == x_stdstring)
        {
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data_);

            std::vector<std::string> &v(*vp);

            char *pos = buf_;
            std::size_t const vsize = v.size();
            for (std::size_t i = 0; i != vsize; ++i)
            {
                v[i].assign(pos, strlen(pos));
                pos += colSize_;
            }
        }
    }
    else // gotData == false
    {
        // nothing to do here, vectors are truncated anyway
    }
}

void odbc_vector_into_type_backend::resize(std::size_t /*sz*/)
{
    // nothing to do because the vectors keep their size (even at the end of the resultset)
}

std::size_t odbc_vector_into_type_backend::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case x_char:
        {
            std::vector<char> *v = static_cast<std::vector<char> *>(data_);
            sz = v->size();
        }
        break;
    case x_mnsociarraystring:
    {
        MNSociArrayString *v = static_cast<MNSociArrayString*>(data_);
        sz = v->getArraySize();
        break;
    }
	case x_mnsociarraytext:
		{
			MNSociArrayText *v = static_cast<MNSociArrayText*>(data_);
			sz = v->getArraySize();
			break;
		}
    case x_short:
        {
            std::vector<short> *v = static_cast<std::vector<short> *>(data_);
            sz = v->size();
        }
        break;
    case x_integer:
        {
            std::vector<int> *v = static_cast<std::vector<int> *>(data_);
            sz = v->size();
        }
        break;
    case x_long_long:
        {
            std::vector<long long> *v =
                static_cast<std::vector<long long> *>(data_);
            sz = v->size();
        }
        break;
    case x_unsigned_long_long:
        {
            std::vector<unsigned long long> *v =
                static_cast<std::vector<unsigned long long> *>(data_);
            sz = v->size();
        }
        break;
    case x_double:
        {
            std::vector<double> *v
                = static_cast<std::vector<double> *>(data_);
            sz = v->size();
        }
        break;
    case x_stdstring:
        {
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data_);
            sz = v->size();
        }
        break;
    case x_mnsocistring:
    {
        std::vector<MNSociString> *v
            = static_cast<std::vector<MNSociString> *>(data_);
        sz = v->size();
    }
    break;
	case x_mnsocitext:
		{
			std::vector<MNSociText> *v
				= static_cast<std::vector<MNSociText> *>(data_);
			sz = v->size();
		}
		break;
    //case x_stdtm:
    //    {
    //        std::vector<std::tm> *v
    //            = static_cast<std::vector<std::tm> *>(data_);
    //        sz = v->size();
    //    }
    //    break;
    case x_odbctimestamp:
    {
        std::vector<TIMESTAMP_STRUCT> *v
            = static_cast<std::vector<TIMESTAMP_STRUCT> *>(data_);
        sz = v->size();
    }
    break;

    case x_statement: break; // not supported
    case x_rowid:     break; // not supported
    case x_blob:      break; // not supported
    }

    return sz;
}

void odbc_vector_into_type_backend::clean_up()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
