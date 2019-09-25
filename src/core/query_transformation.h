//
// Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_QUERY_TRANSFORMATION_H_INCLUDED
#define SOCI_QUERY_TRANSFORMATION_H_INCLUDED

#include "soci-config.h"
#include <functional>
#include <string>

#if _MSC_VER >= 1910
namespace std {
	template <class Arg, class Result>
	struct unary_function
	{
		typedef Arg argument_type;
		typedef Result result_type;
	};

	template <class Arg1, class Arg2, class Result>
	struct binary_function
	{
		typedef Arg1 first_argument_type;
		typedef Arg2 second_argument_type;
		typedef Result result_type;
	};
};
#endif

namespace soci
{

namespace details
{

// Query transformation is a mechanism that enables user to apply 
// any string-to-string transformation to SQL statement just
// before it is executed.
// Transformation procedure is specified by user,
// be it a function or an arbitrary type as long as it
// defines operator() with the appropriate signature:
// unary function takes any type converible-to std::string
// and returns std::string.

class query_transformation_function 
    : public std::unary_function<std::string const&, std::string>
{
public:
    virtual ~query_transformation_function() {}
    virtual result_type operator()(argument_type a) const = 0;
};

template <typename T>
class query_transformation : public query_transformation_function
{
public:
    query_transformation(T callback)
        : callback_(callback)
    {}

	virtual ~query_transformation() {}
    result_type operator()(argument_type query) const
    {
        return callback_(query);
    }

private:
    T callback_;
};

} // namespace details

} // namespace soci

#endif // SOCI_QUERY_TRANSFORMATION_H_INCLUDED
