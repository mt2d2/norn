#include "type.h"

#include <iostream>
#include <algorithm>

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
	std::vector<Type>::iterator it = std::find(types.begin(), types.end(), new_type);
	if (it != types.end())
		*it = new_type;
	else
		types.push_back(new_type);
}

const Type& TypeFactory::get(Primative key) const
{
	for (std::vector<Type>::const_iterator t = types.begin(); t != types.end(); ++t)
		if (t->get_primative() == key)
			return *t;

	raise_error("couldn't find type by primative key, shouldn't happen!");
	return types[0];
}

const Type& TypeFactory::get(const std::string& key) const
{
	for (std::vector<Type>::const_iterator t = types.begin(); t != types.end(); ++t)
		if (t->get_name() == key)
			return *t;
		
	raise_error("couldn't find type by string key: " + key);
	return types[0];
}
