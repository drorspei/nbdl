//
// Copyright Jason Rice 2016
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef NBDL_WEBUI_DETAIL_DOM_MANIPS_HPP
#define NBDL_WEBUI_DETAIL_DOM_MANIPS_HPP

#include <mpdef/list.hpp>
#include <nbdl/catch.hpp>
#include <nbdl/detail/match_if.hpp>
#include <nbdl/fwd/webui/detail/dom_manips.hpp>
#include <nbdl/ui_helper/params_concat.hpp>
#include <nbdl/ui_helper/path.hpp>
#include <nbdl/ui_spec.hpp>
#include <nbdl/webui/detail/event_receiver.hpp>
#include <nbdl/webui/html.hpp>
#include <nbdl/webui/renderer.hpp>

#include <boost/hana/core/is_a.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/first.hpp>
#include <boost/hana/index_if.hpp>
#include <boost/hana/or.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/transform.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/type.hpp>
#include <boost/hana/unpack.hpp>
#include <boost/hana/value.hpp>
#include <emscripten/val.h>
#include <functional>
#include <utility>

namespace nbdl::webui::detail
{
  namespace hana = boost::hana;
  using namespace hana::literals;
  using js_val = nbdl::detail::js_val;

  // converts a Constant or string to js val
  template <typename T>
  emscripten::val to_json_val(T t)
  {
    if constexpr (hana::is_a<hana::string_tag, T>)
    {
      return emscripten::val(hana::to<char const*>(t));
    }
    else
    {
      static_assert(
        not hana::is_a<char const*, T>
      , "String literals not allowed. Please use constexpr alternative like hana::string."
      );
      return emscripten::val(hana::value(t));
    }
  }

  template <typename Store>
  struct make_nested_renderer_impl_type_from_pair_fn
  {
    template <typename Key, typename FnList>
    constexpr auto operator()(mpdef::pair<Key, FnList>) const
    {
      return hana::type_c<renderer_impl<Store, FnList, hana::true_>>;
    }
  };

  template <typename HtmlTagName>
  struct action_fn<begin, html::tag::element_t, HtmlTagName>
  {
    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p) const
    {
      auto el = emscripten::val::global("document").template
        call<emscripten::val>("createElement", to_json_val(HtmlTagName{}));
      std::forward<ParentElement>(p).template
        call<void>("appendChild", el);
      return el;
    }
  };

  template <typename HtmlTagName>
  struct action_fn<end, html::tag::element_t, HtmlTagName>
  {
    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p) const
    {
      return std::forward<ParentElement>(p)["parentNode"];
    }
  };

  // For one string constant
  template <typename AttributeName, char ...Cs>
  struct action_fn<html::tag::attribute_t, AttributeName, mpdef::list<hana::string<Cs...>>>
  {
    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p) const
    {
      p.template call<void>(
        "setAttribute"
      , to_json_val(AttributeName{})
      , to_json_val(hana::string<Cs...>{})
      );
      return std::forward<ParentElement>(p);
    }
  };

  template <typename AttributeName, typename StringParams>
  struct action_fn<html::tag::attribute_t, AttributeName, StringParams>
  {
    action_fn() = delete;
  };

  template <typename Store, typename AttributeName, typename StringParams>
  struct mut_action_fn<html::tag::attribute_t, Store, AttributeName, StringParams>
  {
    Store store;
    emscripten::val el;

    mut_action_fn(Store s)
      : store(s)
      , el(emscripten::val::undefined())
    { }

    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p)
    {
      el = p;
      update();
      return std::forward<ParentElement>(p);
    }

    void update()
    {
      std::string text_value = ui_helper::params_concat(StringParams{}, std::ref(store).get())
        .to_string();
      el.template call<void>(
        "setAttribute"
      , to_json_val(AttributeName{})
      , emscripten::val(std::move(text_value))
      );
    }
  };

  template <typename Value>
  struct action_fn<html::tag::text_node_t, Value>
  {
    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p) const
    {
      auto el = emscripten::val::global("document").template
        call<emscripten::val>("createTextNode", to_json_val(Value{}));
      p.template call<void>("appendChild", el);
      return std::forward<ParentElement>(p);
    }
  };

  template <typename ...PathNodes>
  struct action_fn<html::tag::text_node_t, ui_spec::path_t<PathNodes...>>
  {
    action_fn() = delete;
  };

  template <typename Store, typename ...PathNodes>
  struct mut_action_fn<html::tag::text_node_t, Store, ui_spec::path_t<PathNodes...>>
  {
    Store store;
    emscripten::val el;
    emscripten::val parent_el;

    mut_action_fn(Store s)
      : store(s)
      , el(emscripten::val::undefined())
      , parent_el(emscripten::val::undefined())
    { }

    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p)
    {
      nbdl::ui_helper::path(ui_spec::path_t<PathNodes...>{}
                          , std::ref(store).get() , [&](auto const& value)
      {
        el = emscripten::val::global("document").template
          call<emscripten::val>("createTextNode", emscripten::val(value));
        p.template call<void>("appendChild", el);
        parent_el = p;
      });
      return std::forward<ParentElement>(p);
    }

    void update()
    {
      nbdl::ui_helper::path(ui_spec::path_t<PathNodes...>{}
                          , std::ref(store).get(), [&](auto const& value)
      {
        auto new_el = emscripten::val::global("document").template
          call<emscripten::val>("createTextNode", emscripten::val(value));
        parent_el.template call<void>("replaceChild", new_el, el);
        el = new_el;
      });
    }
  };

  template <typename T>
  struct match_branch_pred_fn
  {
    template <typename U, typename Spec>
    constexpr auto operator()(mpdef::pair<U, Spec>) const
    {
      return hana::or_(
        hana::type_c<T>    == U{}
      , hana::type_c<void> == U{}
      );
    }
  };

  template <typename ...>
  struct construct_branch_renderers;

  template <typename ...X>
  struct construct_branch_renderers<hana::tuple<X...>>
  {
    template <typename Store>
    static auto apply(Store store)
    {
      return hana::make_tuple(X(store)...);
    }
  };

  template <typename PathSpec, typename Branches>
  struct action_fn<ui_spec::match_tag, PathSpec, Branches>
  {
    action_fn() = delete;
  };

  //  Branches - list<pair<T, list<action...>>...>
  template <typename Store, typename PathSpec, typename Branches>
  struct mut_action_fn<ui_spec::match_tag, Store, PathSpec, Branches>
  {
    using Renderers = typename decltype(
      hana::unpack(
        hana::transform(
          Branches{}
        , make_nested_renderer_impl_type_from_pair_fn<Store>{}
        )
      , hana::template_<hana::tuple>
      )
    )::type;

    Store store;
    emscripten::val parent_el;
    emscripten::val container_el;
    int branch_id;
    Renderers renderers;

    mut_action_fn(Store s)
      : store(s)
      , parent_el(emscripten::val::undefined())
      , container_el(emscripten::val::global("document").template
          call<emscripten::val>("createElement", emscripten::val("span")))
      , branch_id(-1)
      , renderers(construct_branch_renderers<Renderers>::apply(s))
    { }

    void update()
    {
      ui_helper::path(PathSpec{}, std::ref(store).get(), [&](auto const& value)
      {
        using T = typename decltype(hana::typeid_(value))::type;
        constexpr auto index = *hana::index_if(Branches{}, match_branch_pred_fn<T>{});
        if (hana::value(index) != branch_id)
        {
          // Something changed! :D
          if (branch_id != -1)
          {
            container_el.template call<void>("removeChild", container_el["firstChild"]);
          }
          branch_id = hana::value(index);
          renderers[index].render(container_el);
        }
      });
    }

    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p)
    {
      parent_el = p;
      update();
      parent_el.template call<void>("appendChild", container_el);
      return std::forward<ParentElement>(p);
    }
  };

  /*
   * match_if
   */

  template <typename PathSpec, typename Branches>
  struct action_fn<ui_spec::match_if_tag, PathSpec, Branches>
  {
    action_fn() = delete;
  };

  //  Branches - list<pair<T, list<action...>>...>
  template <typename Store, typename PathSpec, typename Branches>
  struct mut_action_fn<ui_spec::match_if_tag, Store, PathSpec, Branches>
  {
    using Renderers = typename decltype(
      hana::unpack(
        hana::transform(
          Branches{}
        , make_nested_renderer_impl_type_from_pair_fn<Store>{}
        )
      , hana::template_<hana::tuple>
      )
    )::type;

    Store store;
    emscripten::val parent_el;
    emscripten::val container_el;
    std::size_t branch_id;
    Renderers renderers;

    mut_action_fn(Store s)
      : store(s)
      , parent_el(emscripten::val::undefined())
      , container_el(emscripten::val::global("document").template
          call<emscripten::val>("createElement", emscripten::val("span")))
      , branch_id(-1)
      , renderers(construct_branch_renderers<Renderers>::apply(s))
    { }

    void update()
    {
      constexpr auto preds = hana::transform(Branches{}, hana::first);

      ui_helper::path(PathSpec{}, std::ref(store).get(), [&](auto const& value)
      {
        nbdl::detail::match_if(preds).resolve(nbdl::detail::wrap_promise([&](auto index)
        {
          if (index != branch_id)
          {
            // Something changed! :D
            if (branch_id != -1)
            {
              container_el.template call<void>("removeChild", container_el["firstChild"]);
            }
            branch_id = index;
            renderers[index].render(container_el);
          }
        }), value);
      });
    }

    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p)
    {
      parent_el = p;
      update();
      parent_el.template call<void>("appendChild", container_el);
      return std::forward<ParentElement>(p);
    }
  };

  /*
   * unsafe_set_inner_html
   */

  template <typename String>
  struct action_fn<html::tag::unsafe_set_inner_html_t, String>
  {
    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p) const
    {
      p.set("innerHTML", to_json_val(String{}));
      return std::forward<ParentElement>(p);
    }
  };

  /*
   * event_attribute
   */
  template <typename AttributeName, typename Handler, typename ...Params>
  struct action_fn<html::tag::event_attribute_t, AttributeName, Handler, Params...>
  {
    action_fn() = delete;
  };

  template <typename Store, typename AttributeName, typename Handler, typename ...Params>
  struct mut_action_fn<html::tag::event_attribute_t, Store, AttributeName, Handler, Params...>
  {
    using ReceiverImpl = event_receiver_impl<Store, Handler, Params...>;
    std::unique_ptr<event_receiver> receiver;

    mut_action_fn(Store s)
      : receiver(make_event_receiver(ReceiverImpl(s)))
    { }

    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p) const
    {
      p.template call<void>(
        "addEventListener"
      , to_json_val(AttributeName{})
      , receiver->virtual_(handler_s)(*receiver).as_val()
      );

      return std::forward<ParentElement>(p);
    }

    void update()
    { }
  };

  /*
   * add_class_if
   */
  template <typename ClassName, typename PathSpec, typename Pred>
  struct action_fn<html::tag::add_class_if_t, ClassName, PathSpec, Pred>
  {
    action_fn() = delete;
  };

  template <typename Store, typename ClassName, typename PathSpec, typename Pred>
  struct mut_action_fn<html::tag::add_class_if_t, Store, ClassName, PathSpec, Pred>
  {
    Store store;
    emscripten::val el;


    mut_action_fn(Store s)
      : store(s)
      , el(emscripten::val::undefined())
    { }

    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p)
    {
      el = p;
      update();
      return std::forward<ParentElement>(p);
    }

    void update()
    {
      ui_helper::path(PathSpec{}, std::ref(store).get(), [&](auto const& value)
      {
        el["classList"].template call<void>(
          Pred{}(value) ? "add" : "remove"
        , to_json_val("class"_s)
        , to_json_val(ClassName{})
        );
      });
    }
  };

  /*
   * add_class_when
   */
  template <typename T, typename ClassName, typename PathSpec>
  struct action_fn<html::tag::add_class_when_t<T>, ClassName, PathSpec>
  {
    action_fn() = delete;
  };

  template <typename Store, typename T, typename ClassName, typename PathSpec>
  struct mut_action_fn<html::tag::add_class_when_t<T>, Store, ClassName, PathSpec>
  {
    Store store;
    emscripten::val el;

    mut_action_fn(Store s)
      : store(s)
      , el(emscripten::val::undefined())
    { }

    template <typename ParentElement>
    decltype(auto) operator()(ParentElement&& p)
    {
      el = p;
      update();
      return std::forward<ParentElement>(p);
    }

    void update()
    {
      bool result = false;

      ui_helper::path(PathSpec{}, std::ref(store).get(), nbdl::match_when<T>([&](auto const&)
      {
        result = true;
      }));

      el["classList"].template call<void>(
        result ? "add" : "remove"
      , to_json_val("class"_s)
      , to_json_val(ClassName{})
      );
    }
  };
}
#endif
