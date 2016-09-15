//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED
#define SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED

#include "soci-backend.h"
#ifdef __GNUC__
#include "sql.h"
#endif

namespace soci
{

// default traits class type_conversion, acts as pass through for row::get()
// when no actual conversion is needed.
template <typename T, typename Enable = void>
struct type_conversion
{
    typedef T base_type;

    static void from_base(base_type const & in, SQLLEN ind, T & out)
    {
        if (ind == SQL_NULL_DATA)
        {
            throw soci_error("Null value not allowed for this type");
        }
        out = in;
    }

    static void to_base(T const & in, base_type & out, SQLLEN & ind)
    {
        out = in;
        ind = 1;
    }
};

} // namespace soci

#endif // SOCI_TYPE_CONVERSION_TRAITS_H_INCLUDED
