#ifndef OPCODE_H
#define OPCODE_H

#include <map>
#include <string>

enum Opcode {
#define op(x) x,
#include "opcode.def"
#undef op
  OPCODE_COUNT
};

std::map<long, std::string> opcode_str_map();
static std::map<long, std::string> opcode_str = opcode_str_map();

#endif
