#include "trace.h"

#include <iostream>
#include <string>
#include <sstream>
#include <stack>

#include "../block.h"
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

static asmjit::host::GpVar pop(asmjit::BaseCompiler &c,
                               std::stack<asmjit::host::GpVar> immStack) {
  if (immStack.size() == 0) {
    // need to load from lang stack!
    return asmjit::host::GpVar(c, asmjit::kVarTypeInt64, "LANG_STACK_");
  } else {
    auto ret = immStack.top();
    immStack.pop();
    return ret;
  }
}

std::stack<asmjit::host::GpVar>
Trace::identifyValuesToRetrieveFromLangStack(asmjit::host::Compiler &c) {
  std::stack<asmjit::host::GpVar> immStack;
  for (auto *i : instructions) {
    switch (i->op) {

    // push 1
    case LIT_INT:
    case LIT_CHAR:
    case LIT_FLOAT:
    case NEW_ARY:
      immStack.push(asmjit::host::GpVar(c, asmjit::kVarTypeInt64, "LIT_"));
      break;

    // memory, push 1
    case LOAD_INT:
    case LOAD_CHAR:
    case LOAD_FLOAT:
    case LOAD_ARY:
      immStack.push(locals[i]);
      break;

    // pop 2, push 1
    case LE_INT:
    case LEQ_INT:
    case GE_INT:
    case GEQ_INT:
    case EQ_INT:
    case NEQ_INT:
    case LE_FLOAT:
    case LEQ_FLOAT:
    case GE_FLOAT:
    case GEQ_FLOAT:
    case EQ_FLOAT:
    case NEQ_FLOAT:
    case ADD_INT:
    case SUB_INT:
    case MUL_INT:
    case DIV_INT:
    case MOD_INT:
    case ADD_FLOAT:
    case SUB_FLOAT:
    case MUL_FLOAT:
    case DIV_FLOAT:
    case MOD_FLOAT:
    case AND_INT:
    case OR_INT: {
      pop(c, immStack);
      pop(c, immStack);
      immStack.push(asmjit::host::GpVar(c, asmjit::kVarTypeInt64, "BIN_"));
    } break;

    // pop 1, push 1
    case LOAD_ARY_ELM_CHAR:
    case LOAD_ARY_ELM_INT:
    case LOAD_ARY_ELM_FLOAT:
    case STRUCT_LOAD_INT:
    case STRUCT_LOAD_FLOAT:
    case STRUCT_LOAD_CHAR: {
      pop(c, immStack);
    } break;

    // pop 2
    case STRUCT_STORE_INT:
    case STRUCT_STORE_FLOAT:
    case STRUCT_STORE_CHAR:
    case STORE_ARY_ELM_CHAR:
    case STORE_ARY_ELM_INT:
    case STORE_ARY_ELM_FLOAT:
      pop(c, immStack);
      pop(c, immStack);
      break;

    // pop 1
    case STORE_ARY:
    case PRINT_INT:
    case PRINT_FLOAT:
    case PRINT_CHAR:
    case PRINT_ARY_CHAR:
    case CPY_ARY_CHAR:
    case TJMP:
    case FJMP:
    case UJMP:
    case MALLOC:
      pop(c, immStack);
      break;

    // no effect
    case CALL:
    case CALL_NATIVE:
    case RTRN:
    case I2F:
    case F2I:
    case LBL:
    case LOOP:
      break;
    }
  }

  return immStack;
}

void Trace::load_locals(asmjit::host::Compiler &c,
                        const asmjit::host::GpVar &sp,
                        const asmjit::host::GpVar &mp) {
  // load locals from memory
  auto memOffset = 0;
  auto lastMemoryOffset = 0;
  for (auto *i : instructions) {
    if (i->op == CALL) {
      lastMemoryOffset =
          reinterpret_cast<Block *>(i->arg.p)->get_memory_slots();
      memOffset += lastMemoryOffset;
    } else if (i->op == RTRN) {
      memOffset -= lastMemoryOffset;
    }

    if (i->op == LOAD_INT || i->op == LOAD_CHAR) {
      std::stringstream ss;
      ss << i->arg.l;

      // TODO memory
      // TODO floats

      locals[i] =
          asmjit::host::GpVar(c, asmjit::kVarTypeInt64,
                              std::string("local_mem_" + ss.str()).c_str());
      c.mov(locals[i], asmjit::host::qword_ptr(mp, (i->arg.l * 8) + memOffset));
    }
  }
}

void Trace::jit() {
  asmjit::JitRuntime runtime;
  asmjit::host::Compiler c(&runtime);
  asmjit::FileLogger logger(stdout);
  c.setLogger(&logger);

  c.addFunc(asmjit::host::kFuncConvHost,
            asmjit::FuncBuilder2<asmjit::FnVoid, int64_t *, int64_t *>());

  asmjit::host::GpVar stack(c, asmjit::kVarTypeIntPtr, "stack");
  asmjit::host::GpVar memory(c, asmjit::kVarTypeIntPtr, "memory");

  c.setArg(0, stack);
  c.setArg(1, memory);

  load_locals(c, stack, memory);

  c.ret();
  c.endFunc();

  c.make();
  // need to release make
}
