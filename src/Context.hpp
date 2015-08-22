#ifndef NBDL_CONTEXT_HPP
#define NBDL_CONTEXT_HPP

#include "LambdaTraits.hpp"
#include "StoreCollection.hpp"

namespace nbdl {

template<typename Server, typename ApiDef>
class Context
{
	Server server;
	StoreCollection<ApiDef> store;
	

	public:

	//todo servers probably wont be copyable
	Context(const Server s) : server(s) {}

	template<typename Path, typename MatchFn1, typename... MatchFns>
	typename LambdaTraits<MatchFn1>::ReturnType read(Path path, MatchFn1 fn1, MatchFns... fns)
	{
		using VariantType = typename StoreCollection<ApiDef>::template VariantType<Path>;

		return store.get(
			[&](const Path path) {
				//request from server
				server.read(path, [&](VariantType value) {
					store.suggestAssign(path, value);
				});
			},
			path, fn1, fns...);
	}
};

}//nbdl

#endif