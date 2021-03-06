#ifndef TYPE_H
#define TYPE_H

#include "vm/common.h"

#include <string>
#include <vector>

class Type {
public:
  enum class Primitive {
    BOOLEAN,
    INT,
    FLOAT,
    CHAR,
    INT_ARY,
    FLOAT_ARY,
    CHAR_ARY,
    ARY,
    VOID,
    COMPLEX
  };

  // default
  Type()
      : name("DEFAULT_TYPE"), size(-1), primative(Primitive::VOID),
        members(std::vector<Type>()), fields(std::vector<std::string>()) {}

  // primative constructor
  Type(const std::string &name, Primitive primative)
      : name(name), primative(primative), members(std::vector<Type>()),
        fields(std::vector<std::string>()) {
    size = 8;
  }

  // complex constructor
  Type(const std::string &name, std::vector<Type> members,
       std::vector<std::string> fields = std::vector<std::string>())
      : name(name), primative(Primitive::COMPLEX), members(members),
        fields(fields) {
    for (const auto &i : members)
      size += i.get_size();
  }

  int offset_of(const std::string &field) const;
  const Type &get_member(const std::string &field) const;

  const std::string &get_name() const { return name; }
  int get_size() const { return size; }
  bool is_primitive() const { return (primative != Primitive::COMPLEX); }
  Primitive get_primitive() const { return primative; }
  bool operator==(const Type &other) const {
    return (this->name == other.name);
  }
  bool operator!=(const Type &other) const {
    return !(this->name == other.name);
  }

private:
  std::string name;
  unsigned int size;
  Primitive primative;
  std::vector<Type> members;
  std::vector<std::string> fields;
};

class TypeFactory {
public:
  static TypeFactory &get_instance() {
    static TypeFactory instance;
    return instance;
  }

  TypeFactory();
  void install(const Type &new_type);
  const Type &get(Type::Primitive key) const;
  const Type &get(const std::string &key) const;

private:
  TypeFactory(TypeFactory const &);
  void operator=(TypeFactory const &);

  std::vector<Type> types;
};

#endif
