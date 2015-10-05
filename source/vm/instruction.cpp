#include "instruction.h"

#include <ostream>

#include "opcode.h"

std::ostream &operator<<(std::ostream &os, const Instruction &i) {
  return os << opcode_str[i.op] << " " << i.arg.l;
}
