#ifndef ENTITYLIST_H
#define ENTITYLIST_H

#include<type_traits>
#include "BackEndPrint.h"

class Entity;

class EntityListBase
{
	public:

	virtual ~EntityListBase() {}

	typedef std::vector<Entity *>::size_type size_type;

	virtual Entity &getRef(const int i) = 0;
	virtual void initWithSize(size_type) = 0;
	virtual size_type size() = 0;


};

template<typename T>
class EntityList : public EntityListBase
{
	static_assert(std::is_base_of<Entity, T>::value, "T must be derived from Entity");

	public:

	EntityListBase::size_type size()
	{
		return vec.size();
	}

	Entity &getRef(const int i)
	{
		return vec.at(i);
	}

	protected:

	void initWithSize(EntityListBase::size_type n)
	{
		vec.clear();
		for (int i = 0; i < n; i++)
			append();
	}

	//BackEnd::Ptr backEnd;

	private:

	//void append() { vec.push_back(T(backEnd)); }
	void append() { vec.emplace_back(BackEnd::Ptr(new BackEndPrint("ListItemBleh"))); }

	std::vector<T> vec;
};

#endif
