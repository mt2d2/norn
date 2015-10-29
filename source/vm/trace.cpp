#include "trace.h"

#if !NOJIT

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <stack>
#include <string>

#include "common.h"
#include "block.h"
#include "program.h"
#include "instruction.h"

Trace::Trace()
    : last_state(Trace::State::ABORT),
      bytecode(std::vector<const Instruction *>()),
      instructions(std::vector<IR>()), traceExits(std::vector<uint64_t>()),
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

  if (i->op == LOOP && bytecode.size() > 0 && i->arg.l != bytecode[0]->arg.l) {
    // encountered another loop, abort
    last_state = Trace::State::ABORT;
    goto exit;
  }

  bytecode.push_back(i);
  last_state = Trace::State::TRACING;

exit:
  return last_state;
}

bool Trace::is_complete() const { return last_state == Trace::State::COMPLETE; }

bool Trace::is_head(const Instruction *i) const {
  return bytecode.size() > 0 && i == bytecode[0];
}

void Trace::debug() const {
  for (auto *i : bytecode) {
    std::cout << *i << std::endl;
  }
}

struct Frame {
  std::stack<Trace::IR *> stack;
  std::map<int64_t, Trace::IR *> memory;
};

void Trace::convertBytecodeToIR() {
  std::stack<Frame> frames;
  frames.push(Frame{});
  Frame &frame = frames.top();

  for (const auto *instr : bytecode) {
    switch (instr->op) {
    case LIT_INT: {
      instructions.emplace_back(
          Trace::IR(Trace::IR::Opcode::LitInt, instr->arg.l));
      frame.stack.push(&instructions.back());
    } break;

    case LOAD_INT: {
      const auto ir = frame.memory.find(instr->arg.l);
      if (ir == frame.memory.end()) {
        instructions.emplace_back(
            Trace::IR(Trace::IR::Opcode::LoadInt, instr->arg.l));
        frame.stack.push(&instructions.back());
      } else {
        frame.stack.push(ir->second);
      }
    } break;

    case STORE_INT: {
      const auto ir = frame.stack.top();
      frame.stack.pop();
      frame.memory[instr->arg.l] = ir;
    } break;

    case ADD_INT:
    case MUL_INT:
    case LE_INT: {
      const auto *ir1 = frame.stack.top();
      frame.stack.pop();
      const auto *ir2 = frame.stack.top();
      frame.stack.pop();

      static const std::map<Opcode, Trace::IR::Opcode> bytecodeToIR{
          {ADD_INT, Trace::IR::Opcode::AddInt},
          {MUL_INT, Trace::IR::Opcode::MulInt},
          {LE_INT, Trace::IR::Opcode::LeInt}};
      const auto irOp = bytecodeToIR.find(static_cast<Opcode>(instr->op));
      if (irOp == bytecodeToIR.end())
        raise_error("unknown conversion from bytecode binop to ir binop");

      instructions.emplace_back(Trace::IR(irOp->second, ir1, ir2));
      frame.stack.push(&instructions.back());
    } break;

    case FJMP: {
    } break;
    case UJMP: {
    } break;

    case CALL: {
      frames.push(Frame{});
      frame = frames.top();
    } break;
    case RTRN: {
      frames.pop();
      frame = frames.top();
    } break;

    case LOOP: { /* pass */
    } break;

    default: { raise_error("unknown op in trace compilation"); } break;
    }
  }
}

void Trace::compile(const bool debug) { convertBytecodeToIR(); }

nativeTraceType Trace::get_native_ptr() const { return nativePtr; }

uint64_t Trace::get_trace_exit(int offset) const { return traceExits[offset]; }

std::map<const Block *, unsigned int> Trace::get_trace_calls() const {
  return calls;
}

void Trace::identify_trace_exits() {
  unsigned int bytecodePosition = 0;

  for (const auto *i : bytecode) {
    if (i->op == FJMP || i->op == TJMP) {
      traceExits.push_back(bytecodePosition);
    }

    ++bytecodePosition;
  }
}

#endif // !NOJIT
