//
// Copyright Jason Rice 2015
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef NBDL_CONTEXT_HPP
#define NBDL_CONTEXT_HPP

#include<memory>
#include "LambdaTraits.hpp"
#include "StoreCollection.hpp"

namespace nbdl {

template<typename Traits>
class Context : public enable_shared_from_this<Traits>
{
	using Ptr = std::shared_ptr<Context>;
	using Client = typename typename Traits::Client;
	using ApiDef = typename typename Traits::ApiDef;

	Client client;
	StoreCollection<ApiDef> store;

	public:

	//todo clients probably wont be copyable
	Context(const Client c) : client(c) {}

	template<typename Path, typename MatchFn1, typename... MatchFns>
	typename LambdaTraits<MatchFn1>::ReturnType read(Path path, MatchFn1 fn1, MatchFns... fns)
	{
		using VariantType = typename StoreCollection<ApiDef>::template VariantType<Path>;

		return store.get(
			[&](const Path path) {
				//request from client
				client.read(path, [&](VariantType&& value) {
					store.suggestAssign(path, std::forward<VariantType>(value));
				});
			},
			path, fn1, fns...);
	}
};

template<
	typename Client,
	typename ApiDef>
struct ContextTraits
{
	using Client = Client;
	using ApiDef = ApiDef;
};

}//nbdl

#endif
