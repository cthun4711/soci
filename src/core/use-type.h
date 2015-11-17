//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_USE_TYPE_H_INCLUDED
#define SOCI_USE_TYPE_H_INCLUDED

#include "soci-backend.h"
#include "type-ptr.h"
#include "exchange-traits.h"
#include "mnsocistring.h"
// std
#include <cstddef>
#include <string>
#include <vector>

namespace soci { namespace details {

class statement_impl;

// this is intended to be a base class for all classes that deal with
// binding input data (and OUT PL/SQL variables)
class SOCI_DECL use_type_base
{
public:
    virtual ~use_type_base() {}

    virtual void bind(statement_impl & st, int & position) = 0;
    virtual void pre_use() = 0;
    virtual void clean_up() = 0;
    virtual std::string to_string() = 0;

    virtual std::size_t size() const = 0;  // returns the number of elements
};

typedef type_ptr<use_type_base> use_type_ptr;

class SOCI_DECL standard_use_type : public use_type_base
{
public:
    standard_use_type(void* data, exchange_type type, SQLLEN* ind,
        bool readOnly)
        : data_(data)
        , type_(type)
        , ind_(ind)
        , readOnly_(readOnly)
        , backEnd_(NULL)
    {
        // FIXME
        //convert_to_base();
    }

    virtual ~standard_use_type();
    virtual void bind(statement_impl & st, int & position);
    virtual void * get_data() { return data_; }

    // conversion hook (from arbitrary user type to base type)
    virtual void convert_to_base() {}
    virtual void convert_from_base() {}

protected:
    virtual void pre_use();

private:
    virtual void clean_up();
    virtual std::size_t size() const { return 1; }
    virtual std::string to_string();

    void* data_;
    exchange_type type_;
    SQLLEN* ind_;
    bool readOnly_;

    standard_use_type_backend* backEnd_;
};

class SOCI_DECL vector_use_type : public use_type_base
{
public:
    vector_use_type(void* data, exchange_type type,
        SQLLEN* ind)
        : data_(data)
        , type_(type)
        , ind_(ind)
        , backEnd_(NULL)
    {}

    ~vector_use_type();

private:
    virtual void bind(statement_impl& st, int & position);
    virtual void pre_use();
    virtual void clean_up();
    virtual std::size_t size() const;
    virtual std::string to_string() {
        return "vector_use_type::to_string not implemented";
    }

    void* data_;
    exchange_type type_;
    SQLLEN* ind_;

    vector_use_type_backend * backEnd_;

    virtual void convert_to_base() {}
};

// implementation for the basic types (those which are supported by the library
// out of the box without user-provided conversions)

template <typename T>
class use_type : public standard_use_type
{
public:  
    use_type(T& t, SQLLEN& ind)
        : standard_use_type(&t,
            static_cast<exchange_type>(exchange_traits<T>::x_type), &ind, false)
    {}
    
    use_type(T const& t, SQLLEN& ind)
        : standard_use_type(const_cast<T*>(&t),
            static_cast<exchange_type>(exchange_traits<T>::x_type), &ind, false)
    {}
};

//template <>
//class use_type<MNSociString> : public standard_use_type
//{
//public:
//    use_type(MNSociString& t, std::string const& name = std::string())
//        : standard_use_type(&t.m_ptrCharData[0],
//        static_cast<exchange_type>(exchange_traits<MNSociString>::x_type), false, name)
//    {}
//
//    use_type(MNSociString const& t, std::string const& name = std::string())
//        : standard_use_type(const_cast<MNSociString*>(&t),
//        static_cast<exchange_type>(exchange_traits<MNSociString>::x_type), true, name)
//    {}
//
//    use_type(MNSociString& t, indicator& ind, std::string const& name = std::string())
//        : standard_use_type(&t,
//        static_cast<exchange_type>(exchange_traits<MNSociString>::x_type), ind, false, name)
//    {}
//
//    use_type(MNSociString const& t, indicator& ind, std::string const& name = std::string())
//        : standard_use_type(const_cast<MNSociString*>(&t),
//        static_cast<exchange_type>(exchange_traits<MNSociString>::x_type), ind, false, name)
//    {}
//};

template <typename T>
class use_type<std::vector<T> > : public vector_use_type
{
public:   
    use_type(std::vector<T>& v, std::vector<SQLLEN> const& ind)
        : vector_use_type(&v,
        static_cast<exchange_type>(exchange_traits<T>::x_type), (SQLLEN*)&ind[0])
    {}
    
    use_type(std::vector<T> const& v, std::vector<SQLLEN> const& ind)
        : vector_use_type(const_cast<std::vector<T> *>(&v),
        static_cast<exchange_type>(exchange_traits<T>::x_type), (SQLLEN*)&ind[0])
    {}
};

template <>
class use_type<MNSociArrayString > : public vector_use_type
{
public:
    use_type(MNSociArrayString& v)
        : vector_use_type(&v,
        static_cast<exchange_type>(exchange_traits<MNSociArrayString>::x_type), NULL)
    {}

    use_type(MNSociArrayString const& v)
        : vector_use_type(const_cast<MNSociArrayString *>(&v),
        static_cast<exchange_type>(exchange_traits<MNSociArrayString>::x_type), NULL)
    {}
};

// helper dispatchers for basic types
template <typename T>
use_type_ptr do_use(T & t, SQLLEN & ind, basic_type_tag)
{
    return use_type_ptr(new use_type<T>(t, ind));
}

template <typename T>
use_type_ptr do_use(T const & t, SQLLEN & ind, basic_type_tag)
{
    return use_type_ptr(new use_type<T>(t, ind));
}

template <typename MNSociArrayString>
use_type_ptr do_use(MNSociArrayString & t, basic_type_tag)
{
    return use_type_ptr(new use_type<MNSociArrayString>(t));
}

template <typename T>
use_type_ptr do_use(T & t, std::vector<SQLLEN> & ind, basic_type_tag)
{
    return use_type_ptr(new use_type<T>(t, ind));
}

template <typename T>
use_type_ptr do_use(T const & t, std::vector<SQLLEN> & ind, basic_type_tag)
{
    return use_type_ptr(new use_type<T>(t, ind));
}

} // namespace details

} // namesapce soci

#endif // SOCI_USE_TYPE_H_INCLUDED
