#ifndef BACKEND_H
#define BACKEND_H

#include <string>
#include <memory>
#include <unordered_map>

class BackEnd
{
	public:

		typedef std::shared_ptr<BackEnd> Ptr;
		typedef std::unordered_map<std::string, std::string> ValueMap;

		virtual void validate() {}

		virtual void insert(const ValueMap values) = 0;
		virtual void update(int id, const ValueMap values) = 0;

		virtual bool beginTransaction() { return true; }
		virtual void commit() {}
		virtual void rollback() {}

		//virtual void load() = 0;
};

#endif