//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BACKEND_H_INCLUDED
#define SOCI_BACKEND_H_INCLUDED

#include "soci-config.h"
#include "error.h"
// std
#include <cstddef>
#include <map>
#include <string>
#include <memory>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include "soci-platform.h"
#include <windows.h>
#endif

#include <sqltypes.h>

namespace soci
{

// data types, as seen by the user
enum data_type
{
	dt_string, dt_string_mn_256, dt_string_mn_4000, dt_date, dt_double, dt_integer, dt_long_long, dt_unsigned_long_long, dt_timestamp_struct
};

class session;
class column_properties;

namespace details
{

// data types, as used to describe exchange format
enum exchange_type
{
    x_char,
    x_stdstring,
    x_short,
    x_integer,
    x_long_long,
    x_unsigned_long_long,
    x_double,
    //x_stdtm,
    x_statement,
    x_rowid,
    x_blob,
    x_mnsocistring,
    x_odbctimestamp,
    x_mnsociarraystring,
	x_mnsocitext,
	x_mnsociarraytext,
    x_odbcnumericstruct
};

// type of statement (used for optimizing statement preparation)
enum statement_type
{
    st_one_time_query,
    st_repeatable_query
};

// polymorphic into type backend

class standard_into_type_backend
{
public:
    standard_into_type_backend() {}
    virtual ~standard_into_type_backend() {}

    virtual void define_by_pos(int& position, void* data, exchange_type type, SQLLEN* ind) = 0;

    virtual void pre_fetch() = 0;
    virtual void post_fetch(bool gotData, bool calledFromFetch) = 0;

    virtual void copyIndicatorPointer(SQLLEN* ind) = 0;

    virtual void clean_up() = 0;

private:
    // noncopyable
    standard_into_type_backend(standard_into_type_backend const&);
    standard_into_type_backend& operator=(standard_into_type_backend const&);
};

class vector_into_type_backend
{
public:

    vector_into_type_backend() {}
    virtual ~vector_into_type_backend() {}

    virtual void define_by_pos(int& position, void* data, exchange_type type, SQLLEN* indHolders) = 0;

    virtual void pre_fetch() = 0;
    virtual void post_fetch(bool gotData) = 0;

    virtual void resize(std::size_t sz) = 0;
    virtual std::size_t size() = 0;

    virtual void clean_up() = 0;

private:
    // noncopyable
    vector_into_type_backend(vector_into_type_backend const&);
    vector_into_type_backend& operator=(vector_into_type_backend const&);
};

// polymorphic use type backend

class standard_use_type_backend
{
public:
    standard_use_type_backend() {}
    virtual ~standard_use_type_backend() {}

    virtual void bind_by_pos(int& position, void* data, exchange_type type, bool readOnly, SQLLEN* ind) = 0;

    virtual void pre_use() = 0;

    virtual void clean_up() = 0;

    virtual void copyIndicatorPointer(SQLLEN* ind) = 0;

private:
    // noncopyable
    standard_use_type_backend(standard_use_type_backend const&);
    standard_use_type_backend& operator=(standard_use_type_backend const&);
};

class vector_use_type_backend
{
public:
    vector_use_type_backend() {}
    virtual ~vector_use_type_backend() {}

    virtual void bind_by_pos(int& position, void* data, exchange_type type, SQLLEN* ind) = 0;

    virtual void pre_use() = 0;

    virtual std::size_t size() = 0;

    virtual void clean_up() = 0;

private:
    // noncopyable
    vector_use_type_backend(vector_use_type_backend const&);
    vector_use_type_backend& operator=(vector_use_type_backend const&);
};

// polymorphic statement backend

class statement_backend
{
public:
    statement_backend() {}
    virtual ~statement_backend() {}

    virtual void alloc() = 0;
    virtual void clean_up() = 0;
    virtual void cancel_statement() {}

    virtual void prepare(std::string const& query, statement_type eType) = 0;

    enum exec_fetch_result
    {
        ef_success,
        ef_no_data,
        ef_error
    };

    virtual int execute(int iFetchSize, mn_odbc_error_info& err_info, int iIntoSize = -1) = 0;
    virtual int fetch(int number, mn_odbc_error_info& err_info) = 0;

    virtual long long get_affected_rows() = 0;
    virtual int get_number_of_rows() = 0;

    virtual std::string rewrite_for_procedure_call(std::string const& query) = 0;

    virtual int prepare_for_describe() = 0;
    virtual bool describe_column(int colNum, column_properties& colProperties, mn_odbc_error_info& err_info) = 0;

    virtual standard_into_type_backend* make_into_type_backend() = 0;
    virtual standard_use_type_backend* make_use_type_backend() = 0;
    virtual vector_into_type_backend* make_vector_into_type_backend() = 0;
    virtual vector_use_type_backend* make_vector_use_type_backend() = 0;

private:
    // noncopyable
    statement_backend(statement_backend const&);
    statement_backend& operator=(statement_backend const&);
};

// polymorphic RowID backend

class rowid_backend
{
public:
    virtual ~rowid_backend() {}
};

// polymorphic blob backend

#if _MSC_VER > 1900
class blob_backend
{
public:
    blob_backend() {}
    virtual ~blob_backend() {}

    virtual std::size_t get_len() = 0;
    virtual std::size_t read(std::size_t offset, char* buf,
        std::size_t toRead) = 0;
    virtual std::size_t write(std::size_t offset, char const* buf,
        std::size_t toWrite) = 0;
    virtual std::size_t append(char const* buf, std::size_t toWrite) = 0;
    virtual void trim(std::size_t newLen) = 0;

    virtual std::unique_ptr<std::string> read(mn_odbc_error_info& err_info) = 0;
    virtual void set_data_source(const char* src, const size_t& srcsz) = 0;

private:
    // noncopyable
    blob_backend(blob_backend const&);
    blob_backend& operator=(blob_backend const&);
};
#endif
// polymorphic session backend

class session_backend
{
public:
    session_backend() {}
    virtual ~session_backend() {}

    virtual void begin() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;

    // At least one of these functions is usually not implemented for any given
    // backend as RDBMS support either sequences or auto-generated values, so
    // we don't declare them as pure virtuals to avoid having to define trivial
    // versions of them in the derived classes. However every backend should
    // define at least one of them to allow the code using auto-generated values
    // to work.
    //virtual bool get_next_sequence_value(session&, std::string const&, long&, SQLLEN* )
    //{
    //    return false;
    //}
    //virtual bool get_last_insert_id(session&, std::string const&, long&, SQLLEN* )
    //{
    //    return false;
    //}

    virtual std::string get_backend_name() const = 0;

    virtual statement_backend* make_statement_backend() = 0;
    virtual rowid_backend* make_rowid_backend() = 0;
#if _MSC_VER > 1900
    virtual blob_backend* make_blob_backend() = 0;
#endif

private:
    // noncopyable
    session_backend(session_backend const&);
    session_backend& operator=(session_backend const&);
};

} // namespace details

// simple base class for the session back-end factory

class connection_parameters;

class SOCI_DECL backend_factory
{
public:
    backend_factory() {}
    virtual ~backend_factory() {}

    virtual details::session_backend* make_session(
        connection_parameters const& parameters) const = 0;
};

} // namespace soci

#endif // SOCI_BACKEND_H_INCLUDED
