#ifndef NBDL_ENTITYTRAITS_HPP
#define NBDL_ENTITYTRAITS_HPP

namespace nbdl {

template<typename Impl>
struct EntityTraits;

template<typename T>
struct Void
{
	using type = void;
};
template<class T, class U = void>
struct IsEntity
{
	enum { value = 0 };
};
template<class T>
struct IsEntity<T, typename Void<typename EntityTraits<T>::Members>::type>
{
	enum { value = 1 };
};

template<typename NameFormat, typename Binder, typename Entity>
void bind(Binder &binder, Entity &entity)
{
	EntityTraits<Entity>::Members::template bindMembers<NameFormat>(binder, entity);
}
#include "MemberSet.hpp"
#include "macros/NBDL_ENTITY.hpp"


}//nbdl
#endif
