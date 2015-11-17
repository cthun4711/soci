//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "row.h"

#include <cstddef>
#include <cctype>
#include <sstream>
#include <string>

using namespace soci;
using namespace details;

column_properties::column_properties()
    : dataType_(dt_string), colSize_(0), decDigits_(0), isNullable_(false)
{
}

column_properties::column_properties(const column_properties& obj)
{
    *this = obj;
}

column_properties&
column_properties::operator = (const column_properties& obj)
{
    strcpy((char*)&name_[0], (char*)&obj.name_[0]);
    dataType_ = obj.dataType_;
    colSize_ = obj.colSize_;
    decDigits_ = obj.decDigits_;
    isNullable_ = obj.isNullable_;

    return *this;
}

SQLCHAR*            column_properties::get_name()            { return &name_[0]; }
data_type           column_properties::get_data_type() const { return dataType_; }
SQLULEN&            column_properties::get_column_size()     { return colSize_; }
SQLSMALLINT&        column_properties::get_decimal_digits()  { return decDigits_; }
bool                column_properties::get_is_nullable() const { return isNullable_; }

void column_properties::set_name(char* name) { strcpy((char*)&name_[0], name); }
void column_properties::set_column_size(SQLULEN colSize) { colSize_ = colSize; }
void column_properties::set_decimal_digits(SQLSMALLINT decDigits) { decDigits_ = decDigits; }
void column_properties::set_data_type(data_type dataType)  { dataType_ = dataType; }
void column_properties::set_is_nullable(bool bIsNullable) { isNullable_ = bIsNullable; }

row::row()
    : currentPos_(0)
    , row_size_(0)
{}

row::~row()
{
    clean_up();
}

void row::add_properties(column_properties* cp)
{
    columns_.push_back(cp);
}

const std::size_t& row::size() const
{
    return row_size_;
    //return holders_.size();
}

void row::clean_up()
{
    std::size_t const hsize = holders_.size();
    for (std::size_t i = 0; i != hsize; ++i)
    {
        delete holders_[i];
        delete indicators_[i];
        delete columns_[i];
    }

    columns_.clear();
    holders_.clear();
    indicators_.clear();
    row_size_ = 0;
}

SQLLEN row::get_indicator(const std::size_t& pos) const
{
    return *indicators_[pos];
}

data_type row::getDatatypeForColumn(const std::size_t& pos) const
{
    return columns_[pos]->get_data_type();
}

column_properties* row::get_properties(const std::size_t& pos) const
{
    return columns_[pos];
}

