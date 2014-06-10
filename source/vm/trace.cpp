#include "trace.h"

#include <iostream>
#include <string>
#include <sstream>
#include <stack>

#include "../common.h"
#include "../block.h"
#include "instruction.h"

Trace::Trace()
    : instructions(std::vector<const Instruction *>()), nativePtr(nullptr),
      rootFunctionSize(0), callDepth(0) {}

Trace::~Trace() {
  if (this->nativePtr)
    runtime.release((void *)this->nativePtr);
}

void Trace::record(const Instruction *i) {
  if (!is_head(i))
    instructions.push_back(i);

  if (i->op == CALL)
    ++callDepth;
  else if (i->op == RTRN)
    --callDepth;

  if (i->op != RTRN && callDepth == 0)
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

void Trace::identify_locals(asmjit::host::Compiler &c) {
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
      std::stringstream ss;
      ss << i->arg.l;

      if (locals.find(i->arg.l) == locals.end()) {
        locals[i->arg.l] = LangLocal{
            static_cast<unsigned int>(i->arg.l), memOffset,
            asmjit::host::GpVar(c, asmjit::kVarTypeInt64,
                                std::string("local_mem_" + ss.str()).c_str())};
      }
    }
  }
}

void Trace::load_locals(asmjit::host::Compiler &c,
                        const asmjit::host::GpVar &mp) {
  for (auto &kv : locals) {
    auto local = kv.second;
    c.mov(local.cVar,
          asmjit::host::qword_ptr(mp, (local.memPosition * 8) +
                                          local.memOffsetPosition * 8));
  }
}

void Trace::store_locals(asmjit::host::Compiler &c,
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

  asmjit::host::Compiler c(&runtime);
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

  identify_locals(c);
  load_locals(c, memory);

  c.bind(L_traceEntry);

  for (auto *i : instructions) {
    switch (i->op) {
    case LIT_INT: {
      auto a = asmjit::host::GpVar(c, asmjit::kVarTypeInt64, "LIT_");
      c.mov(a, i->arg.l);
      immStack.push(a);
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
      auto ret = asmjit::host::GpVar(c, asmjit::kVarTypeInt64, "LE_");

      asmjit::Label L_LT = c.newLabel();
      asmjit::Label L_Exit = c.newLabel();

      c.cmp(a, b);
      c.jl(L_LT);

      c.mov(ret, 0);
      c.jmp(L_Exit);

      c.bind(L_LT);
      c.mov(ret, 1);
      c.bind(L_Exit);

      immStack.push(ret);
    } break;

    case ADD_INT: {
      auto a = pop(c, immStack);
      auto b = pop(c, immStack);
      c.add(a, b);
      immStack.push(a);
    } break;

    case FJMP: {
      auto a = pop(c, immStack);
      c.cmp(a, 0);
      c.jz(L_traceExit);
    } break;

    case UJMP: {
      c.comment("jmp causes a crash here :(");
      auto a = asmjit::host::GpVar(c, asmjit::kVarTypeInt64, "TMP");
      c.xor_(a, a);
      c.cmp(a, 0);
      c.jz(L_traceEntry);
      c.unuse(a);
    } break;

    case LOOP:
    case CALL:
    case RTRN:
      break;

    default:
      raise_error("unhandled opcode");
      break;
    }
  }

  c.bind(L_traceExit);

  if (immStack.size() > 0)
    raise_error("need to convert to stack layout");

  store_locals(c, memory);

  c.ret();
  c.endFunc();

  this->nativePtr = asmjit_cast<nativeTraceType>(c.make());
}
