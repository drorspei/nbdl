//
// Copyright Jason Rice 2016
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef NBDL_CONCEPT_STATE_CONSUMER_HPP
#define NBDL_CONCEPT_STATE_CONSUMER_HPP

#include <nbdl/fwd/concept/StateConsumer.hpp>
#include <nbdl/notify_state_change.hpp>

#include <boost/hana/core/default.hpp>

namespace nbdl
{
  namespace hana = boost::hana;

  template<typename T>
  struct StateConsumer
  {
    using Tag = typename hana::tag_of<T>::type;
    static constexpr bool value =
          !hana::is_default<nbdl::notify_state_change_impl      <Tag>>::value
      ;
  };
}

#endif
