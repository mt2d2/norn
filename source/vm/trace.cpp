#include "trace.h"

#if !NOJIT

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

#include "../common.h"
#include "../block.h"
#include "instruction.h"

Trace::Trace(asmjit::JitRuntime *runtime)
    : last_state(Trace::State::ABORT), runtime(runtime),
      instructions(std::vector<const Instruction *>()),
      traceExits(std::vector<uint64_t>()), calls(std::vector<const Block *>()),
      nativePtr(nullptr) {}

Trace::~Trace() {
  if (nativePtr != nullptr)
    runtime->release(reinterpret_cast<void *>(nativePtr));
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

void Trace::compile(const bool debug) {
  identify_trace_exits();
  identify_trace_calls();
  jit(debug);
}

nativeTraceType Trace::get_native_ptr() const { return nativePtr; }

std::vector<uint64_t> Trace::get_trace_exits() const { return traceExits; }

std::vector<const Block *> Trace::get_trace_calls() const { return calls; }

void Trace::identify_trace_exits() {
  unsigned int bytecodePosition = 0;

  for (const auto *i : instructions) {
    if (i->op == FJMP || i->op == TJMP) {
      traceExits.push_back(bytecodePosition);
    }

    ++bytecodePosition;
  }
}

void Trace::identify_trace_calls() {
  // TODO need to count how many instructions into a call to properly restore
  // frame!
  for (const auto *i : instructions) {
    if (i->op == CALL) {
      calls.push_back(reinterpret_cast<Block *>(i->arg.p));
    }
  }
}

static asmjit::host::GpVar
pop(asmjit::BaseCompiler &c,
    std::vector<std::pair<const Instruction *, asmjit::host::GpVar>> &
        immStack) {
  if (immStack.size() == 0) {
    // need to load from lang stack!
    raise_error("implement lang stack access");
    return asmjit::host::GpVar(c, asmjit::kVarTypeInt64, "LANG_STACK_");
  } else {
    auto ret = immStack.back();
    immStack.pop_back();
    return ret.second;
  }
}

std::map<int64_t, asmjit::host::GpVar>
Trace::identify_literals(asmjit::host::Compiler &c) {
  std::map<int64_t, asmjit::host::GpVar> locals;

  for (const auto *i : instructions) {
    if (i->op == LIT_INT) {
      locals[i->arg.l] = asmjit::host::GpVar(
          c, asmjit::kVarTypeInt64,
          std::string("literal_" + std::to_string(i->arg.l)).c_str());
    }
  }

  return locals;
}

void
Trace::load_literals(const std::map<int64_t, asmjit::host::GpVar> &literals,
                     asmjit::host::Compiler &c) {
  c.comment("loading literals");

  for (const auto &kv : literals) {
    auto literalConst = kv.first;
    auto literalReg = kv.second;
    c.mov(literalReg, literalConst);
  }
}

std::map<int64_t, LangLocal> Trace::identify_locals(asmjit::host::Compiler &c) {
  std::map<int64_t, LangLocal> locals;
  unsigned int memOffset = 0;
  unsigned int lastMemoryOffset = 0;

  for (auto *i : instructions) {
    if (i->op == CALL) {
      lastMemoryOffset =
          reinterpret_cast<Block *>(i->arg.p)->get_memory_slots();
      memOffset += lastMemoryOffset;
    } else if (i->op == RTRN) {
      memOffset -= lastMemoryOffset;
    }

    // TODO memory, floats
    if (i->op == LOAD_FLOAT || i->op == STRUCT_LOAD_INT)
      raise_error("unimplemented memory access");

    if (i->op == LOAD_INT || i->op == LOAD_CHAR) {
      if (locals.find(i->arg.l) == locals.end()) {
        locals[i->arg.l] = LangLocal{
            static_cast<unsigned int>(i->arg.l), memOffset,
            asmjit::host::GpVar(
                c, asmjit::kVarTypeInt64,
                std::string("local_" + std::to_string(i->arg.l)).c_str())};
      }
    }
  }

  return locals;
}

void Trace::load_locals(const std::map<int64_t, LangLocal> &locals,
                        asmjit::host::Compiler &c,
                        const asmjit::host::GpVar &mp) {
  c.comment("loading locals");

  for (auto &kv : locals) {
    auto local = kv.second;
    c.mov(local.cVar,
          asmjit::host::qword_ptr(mp, (local.memPosition * 8) +
                                          local.memOffsetPosition * 8));
  }
}

void Trace::restore_locals(const std::map<int64_t, LangLocal> &locals,
                           asmjit::host::Compiler &c,
                           const asmjit::host::GpVar &mp) {
  c.comment("restoring locals");

  // todo, don't restore locals past exit taken
  for (auto &kv : locals) {
    auto local = kv.second;
    c.mov(asmjit::host::qword_ptr(mp, (local.memPosition * 8) +
                                          local.memOffsetPosition * 8),
          local.cVar);
  }
}

void Trace::restore_stack(
    const std::vector<std::vector<
        std::pair<const Instruction *, asmjit::host::GpVar>>> &stackMap,
    asmjit::host::Compiler &c, const asmjit::host::GpVar &traceExitPtr,
    const asmjit::host::GpVar &stack, const asmjit::host::GpVar &stackAdjust) {
  c.comment("restoring stack");

  asmjit::Label L_restorationComplete = c.newLabel();

  asmjit::host::GpVar traceExit(c, asmjit::kVarTypeInt64, "traceExit");
  asmjit::host::GpVar traceExits(c, asmjit::kVarTypeIntPtr, "traceExits");
  asmjit::host::GpVar bytecodeExit(c, asmjit::kVarTypeInt64, "bytecodeExit");
  asmjit::host::GpVar totalStackAdjustment(c, asmjit::kVarTypeInt64,
                                           "totalStackAdjustment");
  asmjit::host::GpVar instructionCounter(c, asmjit::kVarTypeInt64,
                                         "instructionCounter");

  c.mov(traceExit, qword_ptr(traceExitPtr));
  c.mov(traceExits,
        asmjit::imm(reinterpret_cast<uintptr_t>(this->traceExits.data())));
  c.mov(bytecodeExit, qword_ptr(traceExits, traceExit));
  c.unuse(traceExit);
  c.unuse(traceExits);

  c.xor_(instructionCounter, instructionCounter);
  c.xor_(totalStackAdjustment, totalStackAdjustment);

  assert(stackMap.size() == get_trace_exits().size());
  std::vector<asmjit::Label> compensationBlockLabels;
  for (int i = 0; i < stackMap.size(); ++i)
    compensationBlockLabels.push_back(c.newLabel());

  c.comment("identifying compensation code jump");
  for (int i = 0; i < stackMap.size(); ++i) {
    c.cmp(traceExit, i);
    c.jz(compensationBlockLabels[i]);
  }

  auto guardCompensationCount = 0;
  for (const auto &snapshot : stackMap) {
    c.bind(compensationBlockLabels[guardCompensationCount]);
    c.comment(("guard compensation " + std::to_string(guardCompensationCount))
                  .c_str());

    for (const auto *i : instructions) {
      auto snapIt = find_if(
          snapshot.begin(), snapshot.end(),
          [&i](const std::pair<const Instruction *, asmjit::host::GpVar> &v) {
            return v.first == i;
          });

      if (snapIt != snapshot.end() || is_condition(i->op)) {
        c.cmp(bytecodeExit, instructionCounter);
        c.jz(L_restorationComplete);

        c.inc(totalStackAdjustment);

        if (is_condition(i->op)) {
          // TODO, hack, find real value of LE_INT
          // the real value of LE_INT is also related to how we should jump
          // for FJMP and TJMP, and must be recorded from interpreter
          c.mov(qword_ptr(stack, totalStackAdjustment, 3), 0);
        } else {
          c.mov(qword_ptr(stack, totalStackAdjustment, 3), (*snapIt).second);
        }
      }
    }

    guardCompensationCount++;
  }

  c.bind(L_restorationComplete);
  c.mov(qword_ptr(stackAdjust), totalStackAdjustment);
}

void Trace::jit(bool debug) {
  std::vector<std::pair<const Instruction *, asmjit::host::GpVar>> immStack;
  std::vector<std::vector<std::pair<const Instruction *, asmjit::host::GpVar>>>
  stackMap;

  asmjit::host::Compiler c(runtime);
  asmjit::FileLogger logger(stdout);
  if (debug)
    c.setLogger(&logger);

  c.addFunc(asmjit::host::kFuncConvHost,
            asmjit::FuncBuilder4<asmjit::FnVoid, int64_t *, int64_t *,
                                 int64_t *, int64_t *>());
  c.comment("trace");

  asmjit::host::GpVar traceExit(c, asmjit::kVarTypeIntPtr, "traceExit");
  asmjit::host::GpVar stackAdjust(c, asmjit::kVarTypeIntPtr, "stackAdjust");
  asmjit::host::GpVar stack(c, asmjit::kVarTypeIntPtr, "stack");
  asmjit::host::GpVar memory(c, asmjit::kVarTypeIntPtr, "memory");
  c.setArg(0, traceExit);
  c.setArg(1, stackAdjust);
  c.setArg(2, stack);
  c.setArg(3, memory);

  asmjit::Label L_traceEntry = c.newLabel();
  asmjit::Label L_traceExit = c.newLabel();

  auto locals = identify_locals(c);
  load_locals(locals, c, memory);

  auto literals = identify_literals(c);
  load_literals(literals, c);

  c.bind(L_traceEntry);

  unsigned int traceExitNo = 0;
  const Instruction *lastInstruction = nullptr;
  for (const auto *i : instructions) {
    if (is_guard(i->op)) {
      stackMap.push_back(immStack);
    }

    switch (i->op) {
    case LIT_INT: {
      immStack.push_back(std::make_pair(i, literals[i->arg.l]));
    } break;

    case LOAD_INT: {
      immStack.push_back(std::make_pair(i, locals.at(i->arg.l).cVar));
    } break;

    case STORE_INT: {
      auto a = pop(c, immStack);
      locals[i->arg.l].cVar = a;
    } break;

    case LE_INT: {
      auto a = pop(c, immStack);
      auto b = pop(c, immStack);
      c.cmp(a, b);

      // push a dummy value onto stack
      immStack.push_back(std::make_pair(
          i, asmjit::host::GpVar(c, asmjit::kVarTypeInt64, "tmp_le_int")));

    } break;

    case ADD_INT: {
      auto a = pop(c, immStack);
      auto b = pop(c, immStack);
      c.add(a, b);
      immStack.push_back(std::make_pair(i, a));
    } break;

    case FJMP: {
      // pop dummy value off stack
      immStack.pop_back();

      // save the trace exit
      c.mov(qword_ptr(traceExit), traceExitNo++);

      switch (lastInstruction->op) {
      case LE_INT:
        c.jge(L_traceExit);
        break;
      default:
        raise_error("unknown condition in fjmp");
        break;
      }
    } break;

    case UJMP: {
      c.jmp(L_traceEntry);
    } break;

    case LOOP:
    case CALL:
    case RTRN:
      break;

    default:
      raise_error("unhandled opcode");
      break;
    }

    lastInstruction = i;
  }

  c.bind(L_traceExit);

  if (immStack.size() > 0)
    raise_error("need to convert to stack layout");

  restore_locals(locals, c, memory);
  restore_stack(stackMap, c, traceExit, stack, stackAdjust);

  c.ret();
  c.endFunc();

  this->nativePtr = asmjit_cast<nativeTraceType>(c.make());
}

#endif // !NOJIT
