#ifndef NBDL_BINDERS_JSONCPP_READ_HPP
#define NBDL_BINDERS_JSONCPP_READ_HPP

#include<string>
#include<jsoncpp/json/json.h>

namespace nbdl {
namespace binders {
namespace jsoncpp {

class Read
{
	void bind(const std::string, bool &);
	void bind(const std::string, unsigned int &);
	void bind(const std::string, int &);
	void bind(const std::string, double &);
	void bind(const std::string, std::string &);

	Read createObjectReader(const std::string name);

	const Json::Value &jsonVal;

	public:

	template<typename T>
	void bindMember(const std::string name, T &field)
	{
		bind(name, field);
	}

	template<class BinderFn>
	void bindEntity(const std::string name, BinderFn bindFn)
	{
		Read reader = createObjectReader(name);
		bindFn(reader);
	}

	Read(const Json::Value &value);

	template<class EntityType>
	static void fromString(std::string &json, EntityType &entity)
	{
		Json::Reader reader;
		Json::Value root;
		if (!reader.parse(json, root, false))
		{
			throw std::runtime_error("JSON parse error");
		}
		Read r(root);
		entity.bindMembers(r);
	}
			
};

}//jsoncpp
}//binders
}//nbdl

#endif