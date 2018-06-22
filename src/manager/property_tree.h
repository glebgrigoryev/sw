// Copyright (C) 2016-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "property.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;
using ptree = pt::ptree;

SW_SUPPORT_API
std::string ptree2string(const ptree &p);

SW_SUPPORT_API
ptree string2ptree(const std::string &s);

namespace sw
{

namespace detail
{

template <class S, class V>
using typed_iptree = pt::basic_ptree<S, V, pt::detail::less_nocase<S>>;

}

struct PropertyTree : private detail::typed_iptree<std::string, ::sw::PropertyValue>
{
    using base = typename detail::typed_iptree<std::string, ::sw::PropertyValue>;

    typename base::data_type &operator[](const typename base::key_type &k)
    {
        auto i = find(k);
        if (i == not_found())
            return add(k, data_type()).data();
        return i->second.data();
    }

    const typename base::data_type &operator[](const typename base::key_type &k) const
    {
        return get_child(k).data();
    }
};

}
