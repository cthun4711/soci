//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ROW_H_INCLUDED
#define SOCI_ROW_H_INCLUDED

#include "type-holder.h"
#include "soci-backend.h"
#include "type-conversion.h"
// std
#include <cassert>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace soci
{

class SOCI_DECL column_properties
{
    // use getters/setters in case we want to make some
    // of the getters lazy in the future
public:
    column_properties();
    column_properties(const column_properties& obj);

    column_properties& operator= (const column_properties& obj);

    SQLCHAR*            get_name();
    data_type           get_data_type() const;
    SQLULEN&            get_column_size();
    SQLSMALLINT&        get_decimal_digits();
    bool                get_is_nullable() const;

    void set_name(char* name);
    void set_column_size(SQLULEN colSize);
    void set_decimal_digits(SQLSMALLINT decDigits);
    void set_data_type(data_type dataType);
    void set_is_nullable(bool bIsNullable);

private:
    SQLCHAR name_[2048];
    data_type dataType_;
    SQLULEN colSize_;
    SQLSMALLINT decDigits_;
    bool  isNullable_;
};

class SOCI_DECL row
{
public:    
    row();
    ~row();

    void add_properties(column_properties* cp);
    const std::size_t& size() const;
    void clean_up();

    SQLLEN get_indicator(const std::size_t& pos) const;

    template <typename T>
    inline void add_holder(T* t, SQLLEN* ind)
    {
        holders_.push_back(new details::type_holder<T>(t));
        indicators_.push_back(ind);

        row_size_ = holders_.size();
    }

    column_properties*       get_properties(const std::size_t& pos) const;

    data_type                getDatatypeForColumn(const std::size_t& pos) const;

    template <typename T>
    T get_without_cast(std::size_t pos) const
    {
        assert(holders_.size() >= pos + 1);

        return holders_[pos]->get<T>();
    }

    template <typename T>
    T get(const std::size_t& pos) const
    {
        assert(holders_.size() >= pos + 1);

        typedef typename type_conversion<T>::base_type base_type;
        base_type const& baseVal = holders_[pos]->get<base_type>();

        T ret;
        type_conversion<T>::from_base(baseVal, *indicators_[pos], ret);
        return ret;
    }

    template <typename T>
    T get(const std::size_t& pos, T const &nullValue) const
    {
        assert(holders_.size() >= pos + 1);

        if (i_null == *indicators_[pos])
        {
            return nullValue;
        }

        return get<T>(pos);
    }


    template <typename T>
    row const& operator>>(T& value) const
    {
        value = get<T>(currentPos_);
        ++currentPos_;
        return *this;
    }

    void skip(std::size_t num = 1) const
    {
        currentPos_ += num;
    }

    void reset_get_counter() const
    {
        currentPos_ = 0;
    }

private:
    // copy not supported
    row(row const &);
    void operator=(row const &);

    std::vector<column_properties*> columns_;
    std::vector<details::holder*> holders_;
    std::vector<SQLLEN*> indicators_;

    mutable std::size_t currentPos_;
    size_t  row_size_;
};

} // namespace soci

#endif // SOCI_ROW_H_INCLUDED
