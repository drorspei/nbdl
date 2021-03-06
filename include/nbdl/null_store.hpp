//
// Copyright Jason Rice 2016
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef NBDL_NULL_STORE_HPP
#define NBDL_NULL_STORE_HPP

#include <nbdl/fwd/null_store.hpp>

#include <nbdl/apply_message.hpp>
#include <nbdl/get.hpp>
#include <nbdl/tags.hpp>

namespace nbdl
{
  namespace hana = boost::hana;

  struct null_store
  {
    // stores nothing
  };

  template <>
  struct apply_message_impl<null_store>
  {
    template <typename Store, typename Message>
    static constexpr auto apply(Store&&, Message&&)
    {
      // changes nothing
      return hana::false_c;
    }
  };

  template <>
  struct get_impl<null_store>
  {
    template <typename Store, typename Key>
    static constexpr void apply(Store&&, Key&&)
    {
      static_assert(
        std::is_void<Store>::value
      , "Unable to match value in nbdl::null_store"
      );
    }
  };
}

#endif
