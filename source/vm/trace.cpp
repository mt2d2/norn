#include "trace.h"

#if !NOJIT

#include <algorithm>
#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <unordered_set>

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
  std::cout << "Bytecode: " << std::endl;
  for (const auto *i : bytecode) {
    std::cout << *i << std::endl;
  }

  std::cout << "IR: " << std::endl;
  std::size_t i = 1;
  for (const auto &ir : instructions) {
    std::cout << ir << std::endl;
  }
}

struct Frame {
  std::stack<std::size_t> stack;
  std::map<int64_t, std::size_t> memory;
};

void Trace::convertBytecodeToIR() {
  std::stack<Frame> frames;
  frames.push(Frame{});
  Frame &frame = frames.top();

  for (const auto *instr : bytecode) {
    switch (instr->op) {
    case LIT_INT: {
      instructions.emplace_back(IR(IR::Opcode::LitInt, instr->arg.l));
      frame.stack.push(instructions.size());
    } break;

    case LOAD_INT: {
      const auto ir = frame.memory.find(instr->arg.l);
      if (ir == frame.memory.end()) {
        instructions.emplace_back(IR(IR::Opcode::LoadInt, instr->arg.l));
        frame.stack.push(instructions.size());
      } else {
        frame.stack.push(ir->second);
      }
    } break;

    case STORE_INT: {
      const auto ir = frame.stack.top();
      frame.stack.pop();
      frame.memory[instr->arg.l] = ir;
      instructions.emplace_back(IR(IR::Opcode::StoreInt, ir));
    } break;

    case ADD_INT:
    case SUB_INT:
    case MUL_INT:
    case DIV_INT:
    case LE_INT: {
      const auto ir1 = frame.stack.top();
      frame.stack.pop();
      const auto ir2 = frame.stack.top();
      frame.stack.pop();

      static const std::map<Opcode, IR::Opcode> bytecodeToIR{
          {ADD_INT, IR::Opcode::AddInt},
          {SUB_INT, IR::Opcode::SubInt},
          {MUL_INT, IR::Opcode::MulInt},
          {DIV_INT, IR::Opcode::SubInt},
          {LE_INT, IR::Opcode::LeInt}};
      const auto irOp = bytecodeToIR.find(static_cast<Opcode>(instr->op));
      if (irOp == bytecodeToIR.end())
        raise_error("unknown conversion from bytecode binop to ir binop");

      instructions.emplace_back(IR(irOp->second, ir1, ir2));
      frame.stack.push(instructions.size());
    } break;

    case FJMP: {
      const auto ir1 = frame.stack.top();
      frame.stack.pop();

      instructions.emplace_back(
          IR(IR::Opcode::Fjmp, ir1)); /* TODO, add jump location */
    } break;
    case UJMP: {
      instructions.emplace_back(
          IR(IR::Opcode::Ujmp,
             instr->arg.l)); /*todo figure out actual jump position */
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

void Trace::assignVariableName() {
  std::size_t i = 1;
  std::for_each(std::begin(instructions), std::end(instructions),
                [&i](IR &instr) { instr.variableName = i++; });
}

void Trace::propagateConstants() {
  const auto fold = [](IR &instr, const int64_t val) {
    switch (instr.op) {
    case IR::Opcode::AddInt:
      instr.intArg += val;
      break;
    case IR::Opcode::SubInt:
      instr.intArg -= val;
      break;
    case IR::Opcode::MulInt:
      instr.intArg *= val;
      break;
    case IR::Opcode::DivInt:
      instr.intArg /= val;
      break;
    default:
      raise_error("unknown integer fold!");
      break;
    }
    instr.op = IR::Opcode::LitInt;
  };

  enum class WhichRef { Ref1, Ref2 };
  const auto useConstant = [&fold](IR &instr, const WhichRef whichRef,
                                   const int64_t val) {
    if (whichRef == WhichRef::Ref1) {
      instr.hasRef1 = false;
      instr.ref1 = 0;
    } else {
      instr.hasRef2 = false;
      instr.ref2 = 0;
    }

    if (!instr.hasConstantArg1) {
      instr.hasConstantArg1 = true;
      instr.intArg = val;
    } else {
      fold(instr, val);
    }
  };

  std::vector<IR *> worklist;
  for (auto &instr : instructions)
    worklist.push_back(&instr);

  while (!worklist.empty()) {
    const auto *stmt = worklist.back();
    worklist.pop_back();

    if (stmt->yieldsConstant()) // TODO, yieldsConstant should consider bin ops
                                // with only constant operations {
      for (auto &instr : instructions) {
        if (instr.hasRef1 && instr.ref1 == stmt->variableName) {
          useConstant(instr, WhichRef::Ref1, stmt->intArg);
          worklist.push_back(&instr);
        }
        if (instr.hasRef2 && instr.ref2 == stmt->variableName) {
          useConstant(instr, WhichRef::Ref2, stmt->intArg);
          worklist.push_back(&instr);
        }
      }
  }
}

void Trace::deadCodeElimination() {
  std::unordered_set<std::size_t> seenRefs;
  seenRefs.insert(instructions.back().variableName);

  for (auto it = instructions.rbegin(); it != instructions.rend(); ++it) {
    const auto &instr = *it;
    if (seenRefs.count(instr.variableName) != 1 && !instr.hasSideEffect()) {
      instructions.erase(std::next(it).base());
    } else {
      if (instr.hasRef1) {
        seenRefs.insert(instr.ref1);
      }
      if (instr.hasRef2) {
        seenRefs.insert(instr.ref2);
      }
    }
  }
}

void Trace::compile(const bool debug) {
  convertBytecodeToIR();
  assignVariableName();
  propagateConstants();
  deadCodeElimination();
}

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
