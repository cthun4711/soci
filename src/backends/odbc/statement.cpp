//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci-odbc.h"
#include "row.h"
#include <cctype>
#include <sstream>
#include <cstring>
#include <limits>

#ifdef _MSC_VER
// disables the warning about converting int to void*.  This is a 64 bit compatibility
// warning, but odbc requires the value to be converted on this line
// SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)number, 0);
#pragma warning(disable:4312)
#endif

using namespace soci;
using namespace soci::details;


odbc_statement_backend::odbc_statement_backend(odbc_session_backend &session)
    : session_(session), hstmt_(0), numRowsFetched_(0),
      hasVectorUseElements_(false), 
      rowsAffected_(-1LL)
{
}

void odbc_statement_backend::alloc()
{
    SQLRETURN rc;

    // Allocate environment handle
    rc = SQLAllocHandle(SQL_HANDLE_STMT, session_.hdbc_, &hstmt_);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, session_.hdbc_,
            "Allocating statement");
    }

    // Setting so called Server Cursors for the MS SQL Server
    // to be able to use multiple open statements.
    // The default value SQL_CONCUR_READ_ONLY does not allow paralell execution
    // of statements.
    if( session_.get_database_product() == odbc_session_backend::prod_mssql )
    {
        rc = SQLSetStmtAttr(hstmt_, SQL_ATTR_CONCURRENCY, (SQLPOINTER)SQL_CONCUR_ROWVER, 0);
        if (is_odbc_error(rc))
        {
            throw odbc_soci_error(SQL_HANDLE_DBC, session_.hdbc_,
                "Setting statement attribute");
        }
    }
}

void odbc_statement_backend::cancel_statement()
{
    SQLCancel(hstmt_);
}

void odbc_statement_backend::clean_up()
{
    rowsAffected_ = -1LL; 

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt_);
}


void odbc_statement_backend::prepare(std::string const & query,
    statement_type /* eType */)
{
    query_ = query;

    SQLRETURN rc = SQLPrepare(hstmt_, (SQLCHAR*)query_.c_str(), (SQLINTEGER)query_.size());
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_,
                         query_.c_str());
    }
}

int
odbc_statement_backend::execute(int iFetchSize, mn_odbc_error_info& err_info, int iIntoSize)
{
    // Store the number of rows processed by this call.
    SQLULEN rows_processed = 0;
    if (hasVectorUseElements_)
    {
        SQLSetStmtAttr(hstmt_, SQL_ATTR_PARAMS_PROCESSED_PTR, &rows_processed, 0);
    }
    
    // if we are called twice for the same statement we need to close the open
    // cursor or an "invalid cursor state" error will occur on execute
    SQLCloseCursor(hstmt_);
    
    SQLRETURN rc = SQLExecute(hstmt_);
    if( rc == SQL_NEED_DATA )
    {
#if _MSC_VER > 1900
        rc = upload_blobs(err_info);
#endif
    }

    if (is_odbc_error(rc))
    {
        // If executing bulk operation a partial 
        // number of rows affected may be available.
        //if (hasVectorUseElements_)
        //{
        //    rowsAffected_ = 0;

        //    do
        //    {
        //        SQLLEN res = 0;
        //        // SQLRowCount will return error after a partially executed statement.
        //        // SQL_DIAG_ROW_COUNT returns the same info but must be collected immediatelly after the execution.
        //        rc = SQLGetDiagField(SQL_HANDLE_STMT, hstmt_, 0, SQL_DIAG_ROW_COUNT, &res, 0, NULL);
        //        if (!is_odbc_error(rc) && res > 0) // 'res' will be -1 for the where the statement failed.
        //        {
        //            rowsAffected_ += res;
        //        }
        //        --rows_processed; // Avoid unnecessary calls to SQLGetDiagField
        //    }
        //    // Move forward to the next result while there are rows processed.
        //    while (rows_processed > 0 && SQLMoreResults(hstmt_) == SQL_SUCCESS);
        //}
        
        if( err_info.odbc_error_message_.empty() )
        {
            odbc_soci_error myErr(SQL_HANDLE_STMT, hstmt_,
                "Statement Execute");

            err_info.native_error_code_ = myErr.native_error_code();
            err_info.odbc_error_message_ = (char*)myErr.odbc_error_message();
            err_info.odbc_func_name_ = "SQLExecute";
            err_info.odbc_func_returnval_ = rc;
        }

        return -1;
    }
    // We should preserve the number of rows affected here 
    // where we know for sure that a bulk operation was executed.
    else
    {
        rowsAffected_ = 0;

        do {
            SQLLEN res = 0;
            SQLRETURN rc = SQLRowCount(hstmt_, &res);
            if (is_odbc_error(rc))
            {
                odbc_soci_error myErr(SQL_HANDLE_STMT, hstmt_,
                                  "Getting number of affected rows");

                err_info.native_error_code_ = myErr.native_error_code();
                err_info.odbc_error_message_ = (char*)myErr.odbc_error_message();
                err_info.odbc_func_name_ = "SQLRowCount";
                err_info.odbc_func_returnval_ = rc;

                return -1;
            }
            rowsAffected_ += res;
        }
        // Move forward to the next result if executing a bulk operation.
        while (hasVectorUseElements_ && SQLMoreResults(hstmt_) == SQL_SUCCESS);
    }
    SQLSMALLINT colCount;
    SQLNumResultCols(hstmt_, &colCount);

    //check if colCount matches the into buffer!!
    if (colCount != iIntoSize)
    {
        err_info.native_error_code_ = -1;
        err_info.odbc_error_message_ = "SOCI Into Buffer does not match the size of the resultset colum count";
        err_info.odbc_func_name_ = "SQLRowCount";
        err_info.odbc_func_returnval_ = -1;

        return -1;
    }

    if (iFetchSize > 0 && colCount > 0)
    {
        return fetch(iFetchSize, err_info);
    }

    return 1;
}

int
odbc_statement_backend::fetch(int number, mn_odbc_error_info& err_info)
{
    numRowsFetched_ = 0;
    SQLULEN const row_array_size = static_cast<SQLULEN>(number);

    SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0);
    SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)row_array_size, 0);
    SQLSetStmtAttr(hstmt_, SQL_ATTR_ROWS_FETCHED_PTR, &numRowsFetched_, 0);

    SQLRETURN rc = SQLFetch(hstmt_);

    if (SQL_NO_DATA == rc)
    {
        return (int)numRowsFetched_;
    }

    if (is_odbc_error(rc))
    {
        odbc_soci_error myErr(SQL_HANDLE_STMT, hstmt_,
                         "Statement Fetch");

        err_info.native_error_code_ = myErr.native_error_code();
        err_info.odbc_error_message_ = (char*)myErr.odbc_error_message();
        err_info.odbc_func_name_ = "SQLFetch";
        err_info.odbc_func_returnval_ = rc;

        return -1;
    }

    return (int)numRowsFetched_;
}

#if _MSC_VER > 1900
SQLRETURN
odbc_statement_backend::upload_blobs(mn_odbc_error_info& err_info)
{
    SQLRETURN rc;
    SQLPOINTER param;
    while( (rc = SQLParamData(hstmt_, &param)) == SQL_NEED_DATA )
    {
        odbc_blob_backend* bbe = static_cast<odbc_blob_backend*>(param);
        if( bbe )
        {
            rc = bbe->upload(err_info);
            if( rc != SQL_SUCCESS )
                break;
        }
    }

    return rc;
}
#endif

long long odbc_statement_backend::get_affected_rows()
{
    return rowsAffected_;
}

int odbc_statement_backend::get_number_of_rows()
{
    return (int)numRowsFetched_;
}

std::string odbc_statement_backend::rewrite_for_procedure_call(
    std::string const &query)
{
    return query;
}

int odbc_statement_backend::prepare_for_describe()
{
    SQLSMALLINT numCols;
    SQLNumResultCols(hstmt_, &numCols);
    return numCols;
}

bool odbc_statement_backend::describe_column(int colNum, column_properties& colProperties, mn_odbc_error_info& err_info)
{
    SQLSMALLINT colNameBufferOverflow;
    SQLSMALLINT dataType;
    SQLSMALLINT isNullable;

    SQLRETURN rc = SQLDescribeCol(hstmt_, static_cast<SQLUSMALLINT>(colNum),
                                  colProperties.get_name(), 2048,
                                  &colNameBufferOverflow, &dataType,
                                  &colProperties.get_column_size(), &colProperties.get_decimal_digits(), &isNullable);

    if (is_odbc_error(rc))
    {
        odbc_soci_error myErr(SQL_HANDLE_STMT, hstmt_,
                         "describe Column");

        err_info.native_error_code_ = myErr.native_error_code();
        err_info.odbc_error_message_ = (char*)myErr.odbc_error_message();
        err_info.odbc_func_name_ = "SQLDescribeCol";
        err_info.odbc_func_returnval_ = rc;

        return false;
    }

    colProperties.set_is_nullable(isNullable == SQL_NULLABLE);

    data_type type;

    switch (dataType)
    {
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIME:
    case SQL_TYPE_TIMESTAMP:
        type = dt_date;
        break;
    case SQL_NUMERIC:
    case SQL_DECIMAL:
    { //
        if (colProperties.get_decimal_digits() > 0)
        {
            type = dt_double;
        }
        else if (colProperties.get_column_size() <= std::numeric_limits<int>::digits10)
        {
            type = dt_integer;
        }
        else
        {
            type = dt_long_long;
        }
        break;
    }
    case SQL_DOUBLE:
    case SQL_REAL:
    case SQL_FLOAT:
        type = dt_double;
        break;
    case SQL_TINYINT:
    case SQL_SMALLINT:
    case SQL_INTEGER:
        type = dt_integer;
        break;
    case SQL_BIGINT:
        type = dt_long_long;
        break;
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    default:
        type = dt_string;
        break;
    }

    colProperties.set_data_type(type);

    return true;
}

std::size_t odbc_statement_backend::column_size(int colNum)
{
    SQLCHAR colNameBuffer[2048];
    SQLSMALLINT colNameBufferOverflow;
    SQLSMALLINT dataType;
    SQLULEN colSize;
    SQLSMALLINT decDigits;
    SQLSMALLINT isNullable;

    SQLRETURN rc = SQLDescribeCol(hstmt_, static_cast<SQLUSMALLINT>(colNum),
                                  colNameBuffer, 2048,
                                  &colNameBufferOverflow, &dataType,
                                  &colSize, &decDigits, &isNullable);

    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_,
                         "column size");
    }

    return colSize;
}

bool 
odbc_statement_backend::describe_param(int paramNum, mn_odbc_error_info& err_info,
                                    SQLSMALLINT& dataType, SQLULEN& colSize, 
                                    SQLSMALLINT& decDigits, SQLSMALLINT& isNullable)
{

    SQLRETURN rc = SQLDescribeParam(hstmt_, static_cast<SQLUSMALLINT>(paramNum),
    &dataType, &colSize, &decDigits, &isNullable);

    if (is_odbc_error(rc))
    {
        odbc_soci_error myErr(SQL_HANDLE_STMT, hstmt_, "describe parameter");
    
        err_info.native_error_code_ = myErr.native_error_code();
        err_info.odbc_error_message_ = (char*)myErr.odbc_error_message();
        err_info.odbc_func_name_ = "SQLDescribeParam";
        err_info.odbc_func_returnval_ = rc;
    
        return false;
    }

    return true;
}

odbc_standard_into_type_backend * odbc_statement_backend::make_into_type_backend()
{
    return new odbc_standard_into_type_backend(*this);
}

odbc_standard_use_type_backend * odbc_statement_backend::make_use_type_backend()
{
    return new odbc_standard_use_type_backend(*this);
}

odbc_vector_into_type_backend *
odbc_statement_backend::make_vector_into_type_backend()
{
    return new odbc_vector_into_type_backend(*this);
}

odbc_vector_use_type_backend * odbc_statement_backend::make_vector_use_type_backend()
{
    hasVectorUseElements_ = true;
    return new odbc_vector_use_type_backend(*this);
}
