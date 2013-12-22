#include "type.h"

#include <iostream>
#include <algorithm>

int Type::offset_of(const std::string& field) const
{
	int offset = 0;
	int i = 0;

	for (const auto& f : fields)
	{
		if (f == field)
			return offset;

		offset += members[i].get_size();
		i += 1;
	}

	raise_error("unable to get offset of field " + field);
	return 0;
}

const Type& Type::get_member(const std::string& field) const
{
	// int i = 0;
	// for (const auto& f : fields)
	// {
	// 	i++;
	// 	if (f == field)
	// 		return members[i];
	// }


	int i = 0;
	for (const auto& f : fields)
	{
		if (f == field)
			return members[i];
		i++;
	}

	raise_error("unknown field: " + this->get_name() + "." + field);
	return members[0];
}

TypeFactory::TypeFactory()
{
	install(Type("Boolean", BOOLEAN));
	install(Type("Int", INT));
	install(Type("Float", FLOAT));
	install(Type("Char", CHAR));
	install(Type("Ary", ARY));
	install(Type("CharAry", CHAR_ARY));
	install(Type("IntAry", INT_ARY));
	install(Type("FloatArt", FLOAT_ARY));
	install(Type("Void", VOID));
	install(Type("Complex", COMPLEX));
}

void TypeFactory::install(const Type& new_type)
{	
	// type definitions can be overriden
	auto it = std::find(types.begin(), types.end(), new_type);
	if (it != types.end())
		*it = new_type;
	else
		types.push_back(new_type);
}

const Type& TypeFactory::get(Primative key) const
{
	for (const auto& t : types)
		if (t.get_primative() == key)
			return t;

	raise_error("couldn't find type by primative key, shouldn't happen!");
	return types[0];
}

const Type& TypeFactory::get(const std::string& key) const
{
	for (const auto& t : types)
		if (t.get_name() == key)
			return t;

	raise_error("couldn't find type by string key: " + key);
	return types[0];
}
