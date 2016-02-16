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

#ifdef _MSC_VER
// disables the warning about converting int to void*.  This is a 64 bit compatibility
// warning, but odbc requires the value to be converted on this line
// SQLSetStmtAttr(statement_.hstmt_, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)arraySize, 0);
#pragma warning(disable:4312)
#endif

using namespace soci;
using namespace soci::details;



void odbc_vector_use_type_backend::prepare_for_bind(void *&data, SQLUINTEGER &size,
    SQLSMALLINT &sqlType, SQLSMALLINT &cType, SQLLEN* ind, int &position_)
{
    switch (type_)
    {    // simple cases
    case x_short:
        {
            sqlType = SQL_SMALLINT;
            cType = SQL_C_SSHORT;
            size = sizeof(short);
            std::vector<short> *vp = static_cast<std::vector<short> *>(data);
            std::vector<short> &v(*vp);
            data = &v[0];
            arraySize_ = v.size();
        }
        break;
    case x_integer:
        {
            sqlType = SQL_INTEGER;
            cType = SQL_C_SLONG;
            size = sizeof(SQLINTEGER);
            assert(sizeof(SQLINTEGER) == sizeof(int));
            std::vector<int> *vp = static_cast<std::vector<int> *>(data);
            std::vector<int> &v(*vp);
            data = &v[0];
            arraySize_ = v.size();
        }
        break;
    case x_long_long:
        {
            std::vector<long long> *vp =
                static_cast<std::vector<long long> *>(data);
            std::vector<long long> &v(*vp);
            sqlType = SQL_INTEGER;
            cType = SQL_C_SLONG;
            size = sizeof(long long);
            data = &v[0];
            arraySize_ = v.size();
        }
        break;
    case x_unsigned_long_long:
        {
            std::vector<unsigned long long> *vp =
                static_cast<std::vector<unsigned long long> *>(data);
            std::vector<unsigned long long> &v(*vp);

            sqlType = SQL_INTEGER;
            cType = SQL_C_SLONG;
            size = sizeof(unsigned long long);
            data = &v[0];
            arraySize_ = v.size();
        }
        break;
    case x_double:
        {
            sqlType = SQL_DOUBLE;
            cType = SQL_C_DOUBLE;
            size = sizeof(double);
            std::vector<double> *vp = static_cast<std::vector<double> *>(data);
            std::vector<double> &v(*vp);
            data = &v[0];
            arraySize_ = v.size();

            static bool bIsDB2 = statement_.session_.get_database_product() == odbc_session_backend::prod_db2;
            if (bIsDB2)
            {
                mn_odbc_error_info err;
                SQLSMALLINT sqlDataType;
                SQLULEN colSize;
                SQLSMALLINT decDigits;
                SQLSMALLINT isNullable;

                if (statement_.describe_param(position_, err, sqlDataType, colSize, decDigits, isNullable))
                {
                    if (sqlDataType == SQL_DECIMAL && decDigits > 0)
                    {
                        sqlType = SQL_VARCHAR;
                        cType = SQL_C_CHAR;


                        std::size_t maxSize = 257;
                        std::size_t const vecSize = v.size();

                        arraySize_ = vecSize;

                        buf_ = new char[maxSize * vecSize];
                        memset(buf_, 0, maxSize * vecSize);

                        char *pos = buf_;
                        for (std::size_t i = 0; i != vecSize; ++i)
                        {
                            ind[i] = SQL_NTS;

                            snprintf(pos, maxSize-1, "%.*lf", decDigits, v[i]);
                            pos += maxSize;
                        }

                        data = buf_;
                        size = static_cast<SQLINTEGER>(maxSize);
                    }
                }
            }
        }
        break;

    // cases that require adjustments and buffer management
    case x_char:
        {
            std::vector<char> *vp
                = static_cast<std::vector<char> *>(data);
            std::size_t const vsize = vp->size();

            size = sizeof(char) * 2;
            buf_ = new char[size * vsize];

            char *pos = buf_;

            for (std::size_t i = 0; i != vsize; ++i)
            {
                *pos++ = (*vp)[i];
                *pos++ = 0;
            }

            sqlType = SQL_CHAR;
            cType = SQL_C_CHAR;
            data = buf_;
            arraySize_ = vp->size();
        }
        break;
    case x_stdstring:
        {
            sqlType = SQL_CHAR;
            cType = SQL_C_CHAR;

            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data);
            std::vector<std::string> &v(*vp);

            std::size_t maxSize = 0;
            std::size_t const vecSize = v.size();
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                std::size_t sz = v[i].length(); // +1;  // add one for null
                if (sz != 0)
                {
                    ind[i] = static_cast<long>(sz);
                }
                else
                {
                    ind[i] = SQL_NULL_DATA;
                }
                maxSize = sz > maxSize ? sz : maxSize;
            }

            arraySize_ = vecSize;

            buf_ = new char[maxSize * vecSize];
            memset(buf_, 0, maxSize * vecSize);

            char *pos = buf_;
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                if (v[i].empty())
                {
                    strcpy(pos, "");
                }
                else
                {
                    strncpy(pos, v[i].c_str(), v[i].length());
                }
                pos += maxSize;
            }

            data = buf_;
            size = static_cast<SQLINTEGER>(maxSize);
        }
        break;

    case x_mnsociarraystring:
    {
        sqlType = SQL_CHAR;
        cType = SQL_C_CHAR;
        
        MNSociArrayString *v = static_cast<MNSociArrayString *>(data);
        size = 257;
        data = v->getArrayCharData();
        ind = v->getArrayIndicators();
        arraySize_ = v->getCurrentInsertedElementCount();
        break;
    }
    case x_mnsocistring:
        {
            sqlType = SQL_CHAR;
            cType = SQL_C_CHAR;

            std::vector<MNSociString> *vp
                = static_cast<std::vector<MNSociString> *>(data);
            std::vector<MNSociString> &v(*vp);

            //std::size_t maxSize = 0;
            std::size_t const vecSize = v.size();

            //maxSize = 257 + 1;
            //must have size + 1 for the vector iteration!!

            buf_ = new char[256 * vecSize];
            memset(buf_, 0, 256 * vecSize);

            char *pos = buf_;
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                ind[i] = strlen(v[i].m_ptrCharData);
                strncpy(pos, v[i].m_ptrCharData, ind[i]);
                pos += 256;
            }

            data = buf_;
            size = static_cast<SQLINTEGER>(256);
            arraySize_ = vecSize;
        }
        break;                          
   
    case x_odbctimestamp:
    {
        sqlType = SQL_TYPE_TIMESTAMP;
        cType = SQL_C_TYPE_TIMESTAMP;
        size = sizeof(TIMESTAMP_STRUCT);
        std::vector<TIMESTAMP_STRUCT> *vp = static_cast<std::vector<TIMESTAMP_STRUCT> *>(data);
        std::vector<TIMESTAMP_STRUCT> &v(*vp);
        data = &v[0];
        arraySize_ = v.size();
    }
    break;

    case x_statement: break; // not supported
    case x_rowid:     break; // not supported
    case x_blob:      break; // not supported
    }

    colSize_ = size;
    ind_ = ind;
}

void odbc_vector_use_type_backend::bind_helper(int &position, void *data, exchange_type type, SQLLEN* ind)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    SQLSMALLINT sqlType;
    SQLSMALLINT cType;
    SQLUINTEGER size;

    prepare_for_bind(data, size, sqlType, cType, ind, position);

    SQLSetStmtAttr(statement_.hstmt_, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)arraySize_, 0);

    SQLRETURN rc = SQLBindParameter(statement_.hstmt_, static_cast<SQLUSMALLINT>(position++),
                                    SQL_PARAM_INPUT, cType, sqlType, size, 0,
                                    static_cast<SQLPOINTER>(data), size, ind_);

    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, statement_.hstmt_,
            "Error while binding value to column");
    }
}

void odbc_vector_use_type_backend::bind_by_pos(int &position,
    void *data, exchange_type type, SQLLEN* ind)
{
    bind_helper(position, data, type, ind);
}

void odbc_vector_use_type_backend::pre_use()
{
}

std::size_t odbc_vector_use_type_backend::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case x_char:
        {
            std::vector<char> *vp = static_cast<std::vector<char> *>(data_);
            sz = vp->size();
        }
        break;
    case x_mnsociarraystring:
    {
        MNSociArrayString *vp = static_cast<MNSociArrayString *>(data_);
        sz = vp->getCurrentInsertedElementCount(); 
        break;
    }
    case x_short:
        {
            std::vector<short> *vp = static_cast<std::vector<short> *>(data_);
            sz = vp->size();
        }
        break;
    case x_integer:
        {
            std::vector<int> *vp = static_cast<std::vector<int> *>(data_);
            sz = vp->size();
        }
        break;
    case x_long_long:
        {
            std::vector<long long> *vp =
                static_cast<std::vector<long long> *>(data_);
            sz = vp->size();
        }
        break;
    case x_unsigned_long_long:
        {
            std::vector<unsigned long long> *vp =
                static_cast<std::vector<unsigned long long> *>(data_);
            sz = vp->size();
        }
        break;
    case x_double:
        {
            std::vector<double> *vp
                = static_cast<std::vector<double> *>(data_);
            sz = vp->size();
        }
        break;
    case x_stdstring:
        {
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data_);
            sz = vp->size();
        }
        break;
    case x_mnsocistring:
        {
            std::vector<MNSociString> *vp
                = static_cast<std::vector<MNSociString> *>(data_);
            sz = vp->size();
        }
        break;
    //case x_stdtm:
    //    {
    //        std::vector<std::tm> *vp
    //            = static_cast<std::vector<std::tm> *>(data_);
    //        sz = vp->size();
    //    }
    //    break;
    case x_odbctimestamp:
    {
        std::vector<TIMESTAMP_STRUCT> *vp
            = static_cast<std::vector<TIMESTAMP_STRUCT> *>(data_);
        sz = vp->size();
    }
    break;

    case x_statement: break; // not supported
    case x_rowid:     break; // not supported
    case x_blob:      break; // not supported
    }

    return sz;
}

void odbc_vector_use_type_backend::clean_up()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
