#include "trace.h"

#include <iostream>
#include <stack>
#include <string>

#include "../common.h"
#include "../block.h"
#include "instruction.h"

Trace::Trace(asmjit::JitRuntime *runtime)
    : instructions(std::vector<const Instruction *>()), runtime(runtime),
      nativePtr(nullptr), rootFunctionSize(0), callDepth(0) {}

Trace::~Trace() {
  if (nativePtr != nullptr)
    runtime->release(reinterpret_cast<void *>(nativePtr));
}

void Trace::record(const Instruction *i) {
  if (is_head(i))
    return;

  instructions.push_back(i);

  if (i->op == CALL)
    ++callDepth;
  else if (i->op == RTRN)
    --callDepth;

  if (callDepth == 0)
    ++rootFunctionSize;
}

bool Trace::is_head(const Instruction *i) const {
  return instructions.size() > 0 && i == instructions[0];
}

size_t Trace::root_function_size() const { return rootFunctionSize; }

void Trace::debug() const {
  for (auto *i : instructions) {
    std::cout << *i << std::endl;
  }
}

nativeTraceType Trace::get_native_ptr() const { return this->nativePtr; }

static asmjit::host::GpVar pop(asmjit::BaseCompiler &c,
                               std::stack<asmjit::host::GpVar> &immStack) {
  if (immStack.size() == 0) {
    // need to load from lang stack!
    raise_error("implement lang stack access");
    return asmjit::host::GpVar(c, asmjit::kVarTypeInt64, "LANG_STACK_");
  } else {
    auto ret = immStack.top();
    immStack.pop();
    return ret;
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
  for (auto &kv : locals) {
    auto local = kv.second;
    c.mov(local.cVar,
          asmjit::host::qword_ptr(mp, (local.memPosition * 8) +
                                          local.memOffsetPosition * 8));
  }
}

void Trace::store_locals(const std::map<int64_t, LangLocal> &locals,
                         asmjit::host::Compiler &c,
                         const asmjit::host::GpVar &mp) {
  for (auto &kv : locals) {
    auto local = kv.second;
    c.mov(asmjit::host::qword_ptr(mp, (local.memPosition * 8) +
                                          local.memOffsetPosition * 8),
          local.cVar);
  }
}

void Trace::jit(bool debug) {
  std::stack<asmjit::host::GpVar> immStack;

  asmjit::host::Compiler c(runtime);
  asmjit::FileLogger logger(stdout);
  if (debug)
    c.setLogger(&logger);

  c.addFunc(asmjit::host::kFuncConvHost,
            asmjit::FuncBuilder2<asmjit::FnVoid, int64_t *, int64_t *>());

  asmjit::host::GpVar stack(c, asmjit::kVarTypeIntPtr, "stack");
  asmjit::host::GpVar memory(c, asmjit::kVarTypeIntPtr, "memory");
  c.setArg(0, stack);
  c.setArg(1, memory);

  asmjit::Label L_traceEntry = c.newLabel();
  asmjit::Label L_traceExit = c.newLabel();

  auto locals = identify_locals(c);
  load_locals(locals, c, memory);

  auto literals = identify_literals(c);
  load_literals(literals, c);

  c.bind(L_traceEntry);

  const Instruction *lastInstruction = nullptr;
  for (auto *i : instructions) {
    switch (i->op) {
    case LIT_INT: {
      immStack.push(literals[i->arg.l]);
    } break;

    case LOAD_INT: {
      immStack.push(locals.at(i->arg.l).cVar);
    } break;

    case STORE_INT: {
      auto a = pop(c, immStack);
      locals[i->arg.l].cVar = a;
    } break;

    case LE_INT: {
      auto a = pop(c, immStack);
      auto b = pop(c, immStack);
      c.cmp(a, b);
    } break;

    case ADD_INT: {
      auto a = pop(c, immStack);
      auto b = pop(c, immStack);
      c.add(a, b);
      immStack.push(a);
    } break;

    case FJMP: {
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

  store_locals(locals, c, memory);

  c.ret();
  c.endFunc();

  this->nativePtr = asmjit_cast<nativeTraceType>(c.make());
}
