#include "type.h"

#include <iostream>
#include <algorithm>

int Type::offset_of(const std::string &field) const {
  int offset = 0;
  int i = 0;

  for (const auto &f : fields) {
    if (f == field)
      return offset;

    offset += members[i].get_size();
    i += 1;
  }

  raise_error("unable to get offset of field " + field);
  return 0;
}

const Type &Type::get_member(const std::string &field) const {
  int i = 0;
  for (const auto &f : fields) {
    if (f == field)
      return members[i];
    i++;
  }

  raise_error("unknown field: " + this->get_name() + "." + field);
  return members[0];
}

TypeFactory::TypeFactory() {
  install(Type("Boolean", Type::Primative::BOOLEAN));
  install(Type("Int", Type::Primative::INT));
  install(Type("Float", Type::Primative::FLOAT));
  install(Type("Char", Type::Primative::CHAR));
  install(Type("Ary", Type::Primative::ARY));
  install(Type("CharAry", Type::Primative::CHAR_ARY));
  install(Type("IntAry", Type::Primative::INT_ARY));
  install(Type("FloatArt", Type::Primative::FLOAT_ARY));
  install(Type("Void", Type::Primative::VOID));
  install(Type("Complex", Type::Primative::COMPLEX));
}

void TypeFactory::install(const Type &new_type) {
  // type definitions can be overriden
  auto it = std::find(types.begin(), types.end(), new_type);
  if (it != types.end())
    *it = new_type;
  else
    types.push_back(new_type);
}

const Type &TypeFactory::get(Type::Primative key) const {
  for (const auto &t : types)
    if (t.get_primative() == key)
      return t;

  raise_error("couldn't find type by primative key, shouldn't happen!");
  return types[0];
}

const Type &TypeFactory::get(const std::string &key) const {
  for (const auto &t : types)
    if (t.get_name() == key)
      return t;

  raise_error("couldn't find type by string key: " + key);
  return types[0];
}
