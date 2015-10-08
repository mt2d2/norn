#include "trace.h"

#if !NOJIT

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <string>

#include "common.h"
#include "block.h"
#include "program.h"
#include "instruction.h"

Trace::Trace()
    : last_state(Trace::State::ABORT),
      instructions(std::vector<const Instruction *>()),
      traceExits(std::vector<uint64_t>()),
      calls(std::map<const Block *, unsigned int>()), nativePtr(nullptr) {}

Trace::~Trace() {
  if (nativePtr != nullptr)
    raise_error("nativePtr should remain nullptr");
}

Trace::State Trace::record(const Instruction *i) {
  if (is_head(i)) {
    last_state = Trace::State::COMPLETE;
    goto exit;
  }

  if (i->op == LOOP && instructions.size() > 0 &&
      i->arg.l != instructions[0]->arg.l) {
    // encountered another loop, abort
    last_state = Trace::State::ABORT;
    goto exit;
  }

  instructions.push_back(i);
  last_state = Trace::State::TRACING;

exit:
  return last_state;
}

bool Trace::is_complete() const { return last_state == Trace::State::COMPLETE; }

bool Trace::is_head(const Instruction *i) const {
  return instructions.size() > 0 && i == instructions[0];
}

void Trace::debug() const {
  for (auto *i : instructions) {
    std::cout << *i << std::endl;
  }
}

void Trace::compile(const bool debug) {}

nativeTraceType Trace::get_native_ptr() const { return nativePtr; }

uint64_t Trace::get_trace_exit(int offset) const { return traceExits[offset]; }

std::map<const Block *, unsigned int> Trace::get_trace_calls() const {
  return calls;
}

void Trace::identify_trace_exits() {
  unsigned int bytecodePosition = 0;

  for (const auto *i : instructions) {
    if (i->op == FJMP || i->op == TJMP) {
      traceExits.push_back(bytecodePosition);
    }

    ++bytecodePosition;
  }
}

#endif // !NOJIT
