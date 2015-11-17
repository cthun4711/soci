//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BLOB_EXCHANGE_H_INCLUDED
#define SOCI_BLOB_EXCHANGE_H_INCLUDED

#include "blob.h"
#include "into-type.h"
#include "use-type.h"
// std
#include <string>

namespace soci
{

namespace details
{

template <>
class into_type<blob> : public standard_into_type
{
public:
    into_type(blob & b, SQLLEN & ind)
        : standard_into_type(&b, x_blob, &ind) {}
};

template <>
class use_type<blob> : public standard_use_type
{
public:
    use_type(blob & b, SQLLEN & ind)
        : standard_use_type(&b, x_blob, &ind, false) {}
    use_type(blob const & b, SQLLEN & ind)
        : standard_use_type(const_cast<blob *>(&b), x_blob, &ind, true) {}
};

template <>
struct exchange_traits<soci::blob>
{
    typedef basic_type_tag type_family;
};

} // namespace details

} // namespace soci

#endif // SOCI_BLOB_EXCHANGE_H_INCLUDED
