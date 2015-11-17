//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "values.h"
#include "row.h"

#include <cstddef>
#include <map>
#include <sstream>
#include <string>

using namespace soci;
using namespace soci::details;

SQLLEN values::get_indicator(std::size_t pos) const
{
    if (row_)
    {
        return row_->get_indicator(pos);
    }
    else
    {
        return *indicators_[pos];
    }
}

column_properties* values::get_properties(std::size_t pos) const
{
    if (row_)
    {
        return row_->get_properties(pos);
    }

    throw soci_error("Rowset is empty");
}

