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

    column_properties& operator=(const column_properties& obj);

    const std::string&  get_name() const { return name_; }
    data_type           get_data_type() const { return dataType_; }
    unsigned int        get_column_size() const { return colSize_; }
    short               get_decimal_digits() const { return decDigits_; }
    bool                get_is_nullable() const { return isNullable_; }

    void set_name(std::string const& name) { name_ = name; }
    void set_data_type(data_type dataType)  { dataType_ = dataType; }
    void set_column_size(unsigned int iColSize) { colSize_ = iColSize;  }
    void set_decimal_digits(short iDecDigits) { decDigits_ = iDecDigits;  }
    void set_is_nullable(bool bIsNullable) { isNullable_ = bIsNullable; }

private:
    std::string name_;
    data_type dataType_;
    unsigned int colSize_;
    short decDigits_;
    bool  isNullable_;
};

class SOCI_DECL row
{
public:    
    row();
    ~row();

    void uppercase_column_names(bool forceToUpper);
    void add_properties(column_properties const& cp);
    const std::size_t& size() const;
    void clean_up();

    indicator get_indicator(const std::size_t& pos) const;
    indicator get_indicator(std::string const& name) const;

    template <typename T>
    inline void add_holder(T* t, indicator* ind)
    {
        holders_.push_back(new details::type_holder<T>(t));
        indicators_.push_back(ind);

        row_size_ = holders_.size();
    }

    column_properties const& get_properties(const std::size_t& pos) const;
    column_properties const& get_properties(std::string const& name) const;

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
    T get(std::string const &name) const
    {
        std::size_t const pos = find_column(name);
        return get<T>(pos);
    }

    template <typename T>
    T get(std::string const &name, T const &nullValue) const
    {
        std::size_t const pos = find_column(name);

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

    std::size_t find_column(std::string const& name) const;

    std::vector<column_properties> columns_;
    std::vector<details::holder*> holders_;
    std::vector<indicator*> indicators_;
    std::map<std::string, std::size_t> index_;

    bool uppercaseColumnNames_;
    mutable std::size_t currentPos_;
    size_t  row_size_;
};

} // namespace soci

#endif // SOCI_ROW_H_INCLUDED
