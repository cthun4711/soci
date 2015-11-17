//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_INTO_H_INCLUDED
#define SOCI_INTO_H_INCLUDED

#include "into-type.h"
#include "exchange-traits.h"
#include "type-conversion.h"
#include "mnsocistring.h"
// std
#include <cstddef>
#include <vector>

namespace soci
{

// the into function is a helper for defining output variables
// these helpers work with both basic and user-defined types thanks to
// the tag-dispatching, as defined in exchange_traits template

template <typename T>
details::into_type_ptr into(T & t, SQLLEN & ind)
{
    return details::do_into(t, ind,
        typename details::exchange_traits<T>::type_family());
}

template <typename T>
details::into_type_ptr into(T & t, std::vector<SQLLEN> & ind)
{
    return details::do_into(t, ind,
        typename details::exchange_traits<T>::type_family());
}

template <typename MNSociArrayString>
details::into_type_ptr into(MNSociArrayString & t)
{
    return details::do_into<MNSociArrayString>(t, typename details::exchange_traits<MNSociArrayString>::type_family());
}

} // namespace soci

#endif // SOCI_INTO_H_INCLUDED
