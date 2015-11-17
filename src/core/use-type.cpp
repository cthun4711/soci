//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "use-type.h"
#include "statement.h"
#include "mnsocistring.h"

using namespace soci;
using namespace soci::details;

standard_use_type::~standard_use_type()
{
    delete backEnd_;
}

void standard_use_type::bind(statement_impl & st, int & position)
{
    if (backEnd_ == NULL)
    {
        backEnd_ = st.make_use_type_backend();
    }

    backEnd_->bind_by_pos(position, data_, type_, readOnly_, ind_);
}

void standard_use_type::pre_use()   
{
    // Handle IN direction of parameters of SQL statements and procedures
    //convert_to_base();
    backEnd_->copyIndicatorPointer(ind_);
    backEnd_->pre_use();
}

std::string 
standard_use_type::to_string()
{
    std::string strVal;
    char msg[50];

    //convert_to_base();
    switch (this->type_)
    {
    case x_char:
    {
        strVal = "x_char:";
        strVal += (char)data_;
        break;
    }
    case x_stdstring:
    {
        strVal = "x_stdstring:";
        strVal += *(std::string*)data_;
        break;
    }
    case x_mnsocistring:
    {
        strVal = "x_mnsocistring:";
        strVal += ((MNSociString*)data_)->m_ptrCharData;
        break;
        break;
    }
    case x_short:
    {
        sprintf(msg, "%d", *(short*)data_);
        strVal = "x_short:";
        strVal += msg;
        break;
    }
    case x_integer:
    {
        sprintf(msg, "%d", *(int*)data_);
        strVal = "x_integer:";
        strVal += msg;
        break;
    }
    case x_long_long:
    {
        sprintf(msg, "%d", *(long long*)data_);
        strVal = "x_long_long:";
        strVal += msg;
        break;
    }
    case x_unsigned_long_long:
    {
        sprintf(msg, "%d", *(unsigned long long*)data_);
        strVal = "x_unsigned_long_long:";
        strVal += msg;
        break;
    }
    case x_double:
    {
        sprintf(msg, "%f", *(double*)data_);
        strVal = "x_double:";
        strVal += msg;
        break;
    }
    //case x_stdtm:
    //{
    //    std::tm myTime = *(std::tm*)data_;
    //    sprintf(msg, "%d.%d.%d %02d:%02d:%02d", myTime.tm_mday, myTime.tm_mon + 1, myTime.tm_year + 1900, myTime.tm_hour, myTime.tm_min, myTime.tm_sec);
    //    strVal = "x_stdtm:";
    //    strVal += msg;
    //    break;
    //}
    case x_odbctimestamp:
    {
        TIMESTAMP_STRUCT myTime = *(TIMESTAMP_STRUCT*)data_;
        sprintf(msg, "%d.%d.%d %02d:%02d:%02d", myTime.day, myTime.month, myTime.year, myTime.hour, myTime.minute, myTime.second);
        strVal = "x_odbctimestamp:";
        strVal += msg;
        break;
    }
    }

    return strVal;
}

void standard_use_type::clean_up()
{
    if (backEnd_ != NULL)
    {
        backEnd_->clean_up();
    }
}

vector_use_type::~vector_use_type()
{
    delete backEnd_;
}

void vector_use_type::bind(statement_impl & st, int & position)
{
    if (backEnd_ == NULL)
    {
        backEnd_ = st.make_vector_use_type_backend();
    }
    
    backEnd_->bind_by_pos(position, data_, type_,  ind_);
}

void vector_use_type::pre_use()
{
    convert_to_base();

    backEnd_->pre_use();
}

std::size_t vector_use_type::size() const
{
    return backEnd_->size();
}

void vector_use_type::clean_up()
{
    if (backEnd_ != NULL)
    {
        backEnd_->clean_up();
    }
}
