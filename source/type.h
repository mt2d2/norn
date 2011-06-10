#ifndef TYPE_H
#define TYPE_H

#include "vm/common.h"

#include <string>
#include <vector>

enum Primative
{
	BOOLEAN,
	INT,
	FLOAT,
	CHAR,
	CHAR_ARY,
	ARY,
	VOID,
	COMPLEX
};

class Type
{
public:
	// default
	Type() : name("DEFAULT_TYPE"), size(-1), primative(VOID), members(std::vector<Type>()) { }

	// primative constructor
	Type(const std::string& name, Primative primative) : name(name), size(sizeof(primative)), primative(primative), members(std::vector<Type>()) { }

	// complex constructor
	Type(const std::string& name, int size, std::vector<Type> members) : name(name), size(size), primative(COMPLEX), members(members) { }

	const std::string& get_name() { return name; }
	int get_size() { return size; }
	bool is_primative() { return (primative != COMPLEX); }
	Primative get_primative() { return primative; }
	bool operator==(const Type& other) const { return (this->name == other.name); }
	bool operator!=(const Type& other) const { return !(this->name == other.name); }
	
private:
	std::string name;
	int size;
	Primative primative;
	std::vector<Type> members;
};

class TypeFactory
{
public:
	TypeFactory();
	void install(const Type& new_type);
	const Type& get(Primative key);
	const Type& get(const std::string& key);

private:
	std::vector<Type> types;
};

#endif
