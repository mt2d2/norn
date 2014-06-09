#include "trace.h"

#include <iostream>

#include "instruction.h"

Trace::Trace() : instructions(std::vector<const Instruction *>()) {}

Trace::~Trace() {}

void Trace::record(const Instruction *i) {
  if (!is_head(i))
    instructions.push_back(i);
}

bool Trace::is_head(const Instruction *i) {
  return instructions.size() > 0 && i == instructions[0];
}

void Trace::debug() {
  for (auto *i : instructions) {
    std::cout << *i << std::endl;
  }
}
