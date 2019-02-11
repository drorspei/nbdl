//
// Copyright Jason Rice 2017
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef NBDL_WEBUI_DETAIL_JS_EVENT_RECEIVER_HPP
#define NBDL_WEBUI_DETAIL_JS_EVENT_RECEIVER_HPP

#include <mpdef/list.hpp>
#include <nbdl/apply_action.hpp>
#include <nbdl/catch.hpp>
#include <nbdl/detail/default_constructible_lambda.hpp>
#include <nbdl/detail/js_val.hpp>
#include <nbdl/run_sync.hpp>
#include <nbdl/store_compose.hpp>
#include <nbdl/ui_helper/match_params_spec.hpp>
#include <nbdl/webui/html.hpp> // for event_data key

#include <boost/hana/pair.hpp>
#include <boost/hana/string.hpp>
#include <dyno.hpp>
#include <functional>
#include <emscripten.h>
#include <memory>

namespace nbdl::webui::detail
{
  using nbdl::detail::js_val;
  namespace hana = boost::hana;

  constexpr auto receive_event_s  = BOOST_HANA_STRING("receive_event");
  constexpr auto event_data_s     = BOOST_HANA_STRING("event_data");
  constexpr auto handler_s        = BOOST_HANA_STRING("handler");

  struct EventReceiver : decltype(dyno::requires_(
    hana::make_pair(receive_event_s,  dyno::function<void(dyno::T&)>)
  , hana::make_pair(event_data_s,     dyno::function<js_val const&(dyno::T const&)>)
  , hana::make_pair(handler_s,        dyno::function<js_val const&(dyno::T const&)>)
  )) { };

  struct event_receiver_vals
  {
    js_val handler;
    js_val event_data;
  };

  template <typename Store, typename Handler, typename ...Params>
  struct event_receiver_impl
  {
    Store store;
    std::unique_ptr<event_receiver_vals> vals;

    event_receiver_impl(Store s)
      : store(s)
      , vals(new event_receiver_vals{})
    { }

    void receive_event()
    {
      auto store_with_event = nbdl::store_compose(html::event_data, std::ref(vals->event_data), store);
      auto send = [&](auto&& message)
      {
        nbdl::apply_action(
          store
        , std::forward<decltype(message)>(message)
        );
      };

      ui_helper::match_params_spec(store_with_event, mpdef::list<Params...>{}, [&](auto const& ...xs)
      {
        // handler(send, xs...)
        ::nbdl::detail::default_constructible_lambda<Handler>{}(send, std::ref(xs).get()...);
      });
    }
  };

  using event_receiver = dyno::poly<EventReceiver>;

  inline void init_event_receiver(event_receiver const& receiver)
  {
    js_val::init_registry();
    EM_ASM_(
      {
        Module.NBDL_DETAIL_JS_SET(
          $0
        , function(event)
          {
            if (event && event.type === 'submit')
            {
              event.preventDefault();
            }

            Module.NBDL_DETAIL_JS_SET(
              $1
            , event
            );
            Module.__nbdl_webui_detail_event_callback($2);
          }
        );
      }
    , receiver.virtual_(handler_s)(receiver).handle()
    , receiver.virtual_(event_data_s)(receiver).handle()
    , &receiver
    );
  }

  template <typename Impl>
  std::unique_ptr<event_receiver> make_event_receiver(Impl&& impl)
  {
    static_assert(dyno::models<EventReceiver, Impl>);
    event_receiver* temp = new event_receiver{std::forward<Impl>(impl)};
    init_event_receiver(*temp);
    return std::unique_ptr<event_receiver>{temp};
  }

  extern "C"
  {
    inline void EMSCRIPTEN_KEEPALIVE _nbdl_webui_detail_event_callback(void* ptr)
    {
      auto& receiver = *static_cast<event_receiver*>(ptr);
      receiver.virtual_(receive_event_s)(receiver);
    }
  }
}

template <typename T>
constexpr auto ::dyno::default_concept_map<nbdl::webui::detail::EventReceiver, T> =
  dyno::make_concept_map(
    boost::hana::make_pair(
      nbdl::webui::detail::receive_event_s
    , [](T& self) { self.receive_event(); }
    )
  , boost::hana::make_pair(
      nbdl::webui::detail::event_data_s
    , [](T const& self) -> nbdl::detail::js_val const& { return self.vals->event_data; }
    )
  , boost::hana::make_pair(
      nbdl::webui::detail::handler_s
    , [](T const& self) -> nbdl::detail::js_val const& { return self.vals->handler; }
    )
  );

#endif
