#include <algorithm>
#include <stack>
#include <cstring>

#include "block.h"
#include "common.h"
#include "program.h"
#include "memory.h"

#if !NOJIT
#include "AsmJit/AsmJit.h"
using namespace AsmJit;

void putdouble(long l) {
  double n;
  memcpy(&n, &l, sizeof(double));
  printf("%f", n);
}

void putint(int n) { printf("%d", n); }

void Block::jit(const Program &program, Memory &manager,
                unsigned int start_from_ip) {
  const auto &blocks = program.get_blocks();

  Compiler c;
  FileLogger logger(stderr);
  // c.setLogger(&logger);

  // Tell compiler the function prototype we want. It allocates variables
  // representing
  // function arguments that can be accessed through Compiler or Function
  // instance.
  EFunction *func = c.newFunction(
      CALL_CONV_DEFAULT, FunctionBuilder2<Void, int64_t *, int64_t *>());
  c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
  c.comment(std::string("jit function '" + this->get_name() + "'").c_str());

  Label L_Return = c.newLabel();

  // function arguments
  GPVar stack(c.argGP(0));
  GPVar memory(c.argGP(1));

  GPVar stackTop(c.newGP());
  c.mov(stackTop, qword_ptr(stack));
  GPVar memoryTop(c.newGP());
  c.mov(memoryTop, qword_ptr(memory));

  std::map<int, Label> label_positions;
  for (const auto &instr : instructions)
    label_positions[instr.arg.l] = c.newLabel();

  // we need to bounce into the function a bit
  auto bounceToIp = c.newLabel();
  if (start_from_ip > 0) {
    c.comment("bouncing into function");
    GPVar tmp(c.newGP());
    c.xor_(tmp, tmp);
    c.cmp(tmp, 0);
    c.jz(bounceToIp);
    c.unuse(tmp);
  }

  int instr_count = 0;
  for (auto instr = instructions.begin(); instr != instructions.end();
       ++instr) {
    if (start_from_ip > 0 && instr_count == start_from_ip)
      c.bind(bounceToIp);

    auto label_position = label_positions.find(instr_count);
    if (label_position != label_positions.end())
      c.bind(label_position->second);

    ++instr_count;

    switch (instr->op) {
    case LIT_INT:
    case LIT_CHAR: {
      c.comment("LIT_INT/CHAR");

      long lit = 0;
      switch (instr->op) {
      case LIT_INT:
        lit = instr->arg.l;
        break;
      case LIT_CHAR:
        lit = instr->arg.c;
        break;
      }

      c.add(stackTop, 8);
      c.mov(qword_ptr(stackTop), lit);
    } break;

    case LIT_FLOAT: {
      c.comment("LIT_FLOAT");

      GPVar tmp(c.newGP());
      GPVar address(c.newGP());

      c.mov(address, imm((sysint_t)(void *)&instr->arg.d));
      c.mov(tmp, qword_ptr(address));

      c.add(stackTop, 8);
      c.mov(qword_ptr(stackTop), tmp);
    } break;

    case LOAD_INT:
    case LOAD_CHAR:
    case LOAD_FLOAT: {
      c.comment("LOAD_INT/CHAR/FLOAT");
      GPVar tmp(c.newGP());
      c.mov(tmp, qword_ptr(memoryTop, (instr->arg.l * 8)));

      c.add(stackTop, 8);
      c.mov(qword_ptr(stackTop), tmp);

      c.unuse(tmp);
    } break;

    case STORE_INT:
    case STORE_CHAR:
    case STORE_FLOAT:
    case STORE_ARY: {
      c.comment("STORE_INT/CHAR/FLOAT");
      GPVar tmp(c.newGP());
      c.mov(tmp, qword_ptr(stackTop));
      c.sub(stackTop, 8);

      c.mov(qword_ptr(memoryTop, (instr->arg.l * 8)), tmp);

      c.unuse(tmp);
    } break;

    case ADD_INT: {
      c.comment("ADD_INT");

      GPVar tmp(c.newGP());
      c.mov(tmp, qword_ptr(stackTop));

      c.add(qword_ptr(stackTop, -8), tmp);
      c.sub(stackTop, 8);

      c.unuse(tmp);
    } break;

    case MUL_INT:
    case SUB_INT: {
      c.comment("ARITH_INT");
      GPVar tmp1(c.newGP());
      GPVar tmp2(c.newGP());

      c.mov(tmp1, qword_ptr(stackTop));
      c.mov(tmp2, qword_ptr(stackTop, -8));

      switch (instr->op) {
      case MUL_INT:
        c.imul(tmp1, tmp2);
        break;
      case SUB_INT:
        c.sub(tmp1, tmp2);
        break;
      case ADD_INT:
        c.add(tmp1, tmp2);
        break;
      }

      c.sub(stackTop, 8);
      c.mov(qword_ptr(stackTop), tmp1);

      c.unuse(tmp1);
      c.unuse(tmp2);
    } break;

    case DIV_INT:
    case MOD_INT: {
      c.comment("MOD_INT");
      GPVar tmp0(c.newGP());
      GPVar tmp1(c.newGP());
      // GPVar tmp2(c.newGP());

      c.mov(tmp1, qword_ptr(stackTop));

      // make rdx 0, but really should sign extend it with cqo
      GPVar t_rdx(c.newGP());
      c.alloc(t_rdx, rdx);
      c.xor_(t_rdx, t_rdx);
      c.unuse(t_rdx);

      c.idiv(tmp0, tmp1, qword_ptr(stackTop, -8));

      c.sub(stackTop, 8);

      switch (instr->op) {
      case DIV_INT:
        c.mov(qword_ptr(stackTop), tmp1);
        break;
      case MOD_INT:
        c.mov(qword_ptr(stackTop), tmp0);
        break;
      }

      c.unuse(tmp0);
      c.unuse(tmp1);
      c.unuse(t_rdx);
    } break;

    case ADD_FLOAT:
    case SUB_FLOAT:
    case MUL_FLOAT:
    case DIV_FLOAT: {
      c.comment("ARITH_FLOAT");

      XMMVar tmp1(c.newXMM());
      XMMVar tmp2(c.newXMM());

      c.movsd(tmp1, qword_ptr(stackTop));
      c.movsd(tmp2, qword_ptr(stackTop, -8));

      switch (instr->op) {
      case ADD_FLOAT:
        c.addsd(tmp1, tmp2);
        break;
      case SUB_FLOAT:
        c.subsd(tmp1, tmp2);
        break;
      case MUL_FLOAT:
        c.mulsd(tmp1, tmp2);
        break;
      case DIV_FLOAT:
        c.divsd(tmp1, tmp2);
        break;
      }

      c.sub(stackTop, 8);
      c.movsd(qword_ptr(stackTop), tmp1);
    } break;

    case LEQ_INT:
    case LE_INT:
    case GE_INT:
    case GEQ_INT:
    case EQ_INT:
    case NEQ_INT: {
      c.comment("LEQ/LE/GE/GEQ/EQ/NEQ_INT");
      GPVar tmp1(c.newGP());
      GPVar tmp2(c.newGP());

      c.mov(tmp1, qword_ptr(stackTop));
      c.mov(tmp2, qword_ptr(stackTop, -8));

      c.sub(stackTop, 8);

      // branch
      Label L_LT = c.newLabel();
      Label L_Exit = c.newLabel();

      c.cmp(tmp1, tmp2);

      switch (instr->op) {
      case LEQ_INT:
        c.jle(L_LT);
        break;
      case LE_INT:
        c.jl(L_LT);
        break;
      case GE_INT:
        c.jg(L_LT);
        break;
      case GEQ_INT:
        c.jge(L_LT);
        break;
      case EQ_INT:
        c.je(L_LT);
        break;
      case NEQ_INT:
        c.jne(L_LT);
        break;
      }

      c.mov(qword_ptr(stackTop), imm(0));
      c.jmp(L_Exit);

      c.bind(L_LT);
      c.mov(qword_ptr(stackTop), imm(1));

      c.bind(L_Exit);

      c.unuse(tmp1);
      c.unuse(tmp2);
    } break;

    case LEQ_FLOAT:
    case LE_FLOAT:
    case GE_FLOAT:
    case GEQ_FLOAT:
    case EQ_FLOAT:
    case NEQ_FLOAT: {
      c.comment("LEQ/LE/GE/GEQ/EQ/NEQ_FLOAT");
      XMMVar tmp1(c.newXMM());
      XMMVar tmp2(c.newXMM());

      c.movsd(tmp1, qword_ptr(stackTop));
      c.movsd(tmp2, qword_ptr(stackTop, -8));

      c.sub(stackTop, 8);

      // branch
      Label L_LT = c.newLabel();
      Label L_Exit = c.newLabel();

      c.comisd(tmp1, tmp2);

      switch (instr->op) {
      case LEQ_FLOAT:
        c.jb(L_LT);
        break;
      case LE_FLOAT:
        c.jbe(L_LT);
        break;
      case GE_FLOAT:
        c.ja(L_LT);
        break;
      case GEQ_FLOAT:
        c.jae(L_LT);
        break;
      case EQ_FLOAT:
        c.je(L_LT);
        break;
      case NEQ_FLOAT:
        c.jne(L_LT);
        break;
      }

      c.mov(qword_ptr(stackTop), imm(0));
      c.jmp(L_Exit);

      c.bind(L_LT);
      c.mov(qword_ptr(stackTop), imm(1));

      c.bind(L_Exit);

      c.unuse(tmp1);
      c.unuse(tmp2);
    } break;

    case TJMP:
    case FJMP: {
      c.comment("FJMP");
      GPVar tmp(c.newGP());

      c.mov(tmp, qword_ptr(stackTop));
      c.sub(stackTop, 8);

      switch (instr->op) {
      case TJMP:
        c.cmp(tmp, 1);
        break;
      case FJMP:
        c.cmp(tmp, 0);
        break;
      }

      c.jz(label_positions[instr->arg.l]);
      c.unuse(tmp);
    } break;

    case UJMP:
      c.comment("UJMP");
      c.jmp(label_positions[instr->arg.l]);
      break;

    case CALL:
    case CALL_NATIVE: {
      auto callee =
          std::find(blocks.begin(), blocks.end(), (Block *)instr->arg.p);
      if (callee == blocks.end())
        raise_error("couldn't identify block for jit native call");

      if (!(*callee)->native && *callee != this)
        (*callee)->jit(program, manager);

      c.comment(
          std::string("CALL_NATIVE '" + (*callee)->get_name() + "'").c_str());

      c.mov(qword_ptr(stack), stackTop);
      if (this->get_memory_slots() > 0)
        c.add(qword_ptr(memory), 8 * this->get_memory_slots());

      ECall *ctx = nullptr;
      if (*callee == this)
        ctx = c.call(func->getEntryLabel());
      else
        ctx = c.call(imm((sysint_t)(void *)(*callee)->native));

      GPVar ret(c.newGP());

      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder2<int64_t, long *, long *>());
      ctx->setArgument(0, stack);
      ctx->setArgument(1, memory);
      ctx->setReturn(ret);

      c.mov(stackTop, qword_ptr(stack));

      if ((*callee)->get_jit_type() == OPTIMIZING) {
        c.add(stackTop, 8);
        c.mov(qword_ptr(stackTop), ret);
        c.unuse(ret);
      }

      if (this->get_memory_slots() > 0)
        c.sub(qword_ptr(memory), 8 * this->get_memory_slots());
    } break;

    case I2F: {
      c.comment("I2F");
      XMMVar tmp1(c.newXMM());
      c.cvtsi2sd(tmp1, qword_ptr(stackTop));
      c.movsd(qword_ptr(stackTop), tmp1);
      c.unuse(tmp1);
    } break;

    case F2I: {
      c.comment("F2I");
      GPVar tmp(c.newGP());
      c.cvtsd2si(tmp, qword_ptr(stackTop));
      c.mov(qword_ptr(stackTop), tmp);
      c.unuse(tmp);
    } break;

    case RTRN:
      // would be better to have ret here, but weird bugs trying to finish jit'd
      // function
      // is it preferable to have a single exit?
      c.comment("RTRN");
      if ((instr + 1) != instructions.end() ||
          ((instr + 1)->op != RTRN && (instr + 1 + 1) == instructions.end()))
        c.jmp(L_Return);
      break;

    case PRINT_INT:
    case PRINT_CHAR:
    case PRINT_FLOAT: {
      c.comment("PRINT_INT/CHAR/FLOAT");
      GPVar tmp(c.newGP());
      c.mov(tmp, qword_ptr(stackTop));
      c.sub(stackTop, 8);

      void *printFunction = nullptr;
      if (instr->op == PRINT_INT)
        printFunction = reinterpret_cast<void *>(&putint);
      else if (instr->op == PRINT_FLOAT)
        printFunction = reinterpret_cast<void *>(&putdouble);
      else
        printFunction = reinterpret_cast<void *>(&putchar);

      ECall *ctx = c.call(imm((sysint_t)printFunction));
      ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<Void, void *>());
      ctx->setArgument(0, tmp);

      c.unuse(tmp);
    } break;

    case LIT_LOAD_ADD:
    case LIT_LOAD_SUB: {
      c.comment("LIT_LOAD_ADD/SUB");
      GPVar tmp(c.newGP());
      c.mov(tmp, qword_ptr(memoryTop, (instr->arg.l * 8)));

      switch (instr->op) {
      case LIT_LOAD_ADD:
        c.add(tmp, instr->arg2.l);
        break;
      case LIT_LOAD_SUB:
        c.sub(tmp, instr->arg2.l);
        break;
      }

      c.add(stackTop, 8);
      c.mov(qword_ptr(stackTop), tmp);

      c.unuse(tmp);
    } break;

    case LIT_LOAD_LE: {
      c.comment("LIT_LOAD_LE");

      GPVar tmp(c.newGP());
      c.mov(tmp, qword_ptr(memoryTop, (instr->arg.l * 8)));

      Label L_LE = c.newLabel();
      Label L_Exit = c.newLabel();

      c.cmp(tmp, instr->arg2.l);
      c.jl(L_LE);

      c.mov(tmp, 0);
      c.jmp(L_Exit);

      c.bind(L_LE);
      c.mov(tmp, 1);

      c.bind(L_Exit);

      c.add(stackTop, 8);
      c.mov(qword_ptr(stackTop), tmp);

      c.unuse(tmp);
    } break;

    case MALLOC: {
      c.comment("MALLOC");
      GPVar managerReg(c.newGP());
      GPVar memoryBlockTop(c.newGP());
      c.mov(managerReg, (uintptr_t) & manager);
      // c.mov(qword_ptr(stack), stackTop);
      c.mov(memoryBlockTop, qword_ptr(memory));
      c.add(memoryBlockTop, this->get_memory_slots() * 8);

      // manager.set_stack(stack);
      ECall *ctx = c.call(imm((sysint_t) & Memory_set_stack));
      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder2<Void, void *, int64_t *>());
      ctx->setArgument(0, managerReg);
      ctx->setArgument(1, stackTop);

      // manager.set_memory(memory + block->get_memory_slots());
      ctx = c.call(imm((sysint_t) & Memory_set_memory));
      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder2<Void, void *, int64_t *>());
      ctx->setArgument(0, managerReg);
      ctx->setArgument(1, memoryBlockTop);

      // push(reinterpret_cast<uint8_t*>(manager.allocate(pop<int64_t>())));
      GPVar sizeToMalloc(c.newGP());
      c.mov(sizeToMalloc, qword_ptr(stackTop));
      c.sub(stackTop, 8);
      GPVar mallocd(c.newGP());

      ctx = c.call(imm((sysint_t) & Memory_allocate));
      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder2<void *, void *, int64_t>());
      ctx->setArgument(0, managerReg);
      ctx->setArgument(1, sizeToMalloc);
      ctx->setReturn(mallocd);

      c.add(stackTop, 8);
      c.mov(qword_ptr(stackTop), mallocd);

      c.unuse(managerReg);
      c.unuse(memoryBlockTop);
      c.unuse(sizeToMalloc);
      c.unuse(mallocd);
    } break;
    case NEW_ARY: {
      c.comment("NEW_ARY");
      GPVar managerReg(c.newGP());
      c.mov(managerReg, (uintptr_t) & manager);
      GPVar sizeToMalloc(c.newGP());
      c.mov(sizeToMalloc, instr->arg.l);
      GPVar newArray(c.newGP());

      ECall *ctx = c.call(imm((uintptr_t) & Memory_new_lang_array));
      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder2<void *, void *, int>());
      ctx->setArgument(0, managerReg);
      ctx->setArgument(1, sizeToMalloc);
      ctx->setReturn(newArray);

      c.add(stackTop, 8);
      c.mov(qword_ptr(stackTop), newArray);

      c.unuse(managerReg);
      c.unuse(sizeToMalloc);
      c.unuse(newArray);
    } break;
    case CPY_ARY_CHAR: {
      c.comment("CPY_ARY_CHAR");

      GPVar programReg(c.newGP());
      c.mov(programReg, (uintptr_t) & program);

      GPVar stringNumber(c.newGP());
      c.mov(stringNumber, instr->arg.l);

      GPVar str(c.newGP());
      c.mov(str, qword_ptr(stackTop));

      ECall *ctx = c.call(imm((uintptr_t) & Program_copy_array_char));
      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder3<Void, void *, int, void *>());
      ctx->setArgument(0, programReg);
      ctx->setArgument(1, stringNumber);
      ctx->setArgument(2, str);

      c.unuse(stringNumber);
      c.unuse(programReg);
      c.unuse(str);
    } break;
    case PRINT_ARY_CHAR: {
      c.comment("PRINT_ARY_CHAR");

      GPVar str(c.newGP());
      c.mov(str, qword_ptr(stackTop));
      c.sub(stackTop, 8);

      ECall *ctx = c.call(imm((uintptr_t) & Program_print_array_char));
      ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<Void, void *>());
      ctx->setArgument(0, str);

      c.unuse(str);
    } break;
    case STRUCT_STORE_INT: {
      c.comment("STRUCT_STORE_INT");
      // auto s =
      // reinterpret_cast<uint8_t*>(pop<AllocatedMemory*>()->data.getPointer());
      GPVar ptr(c.newGP());
      c.mov(ptr, qword_ptr(stackTop));
      // return asBits & tagMask;
      c.and_(ptr, TaggedPointer<void *, 8>().pointerMask);

      // auto v = pop<int64_t>();
      GPVar value(c.newGP());
      c.mov(value, qword_ptr(stackTop, -8));
      c.sub(stackTop, 16);

      // memcpy(s + instr->arg.l, &v, sizeof(int64_t));
      c.mov(qword_ptr(ptr, instr->arg.l), value);

      c.unuse(ptr);
      c.unuse(value);
    } break;
    case STRUCT_LOAD_INT: {
      c.comment("STRUCT_LOAD_INT");
      // auto s =
      // reinterpret_cast<uint8_t*>(pop<AllocatedMemory*>()->data.getPointer());
      GPVar ptr(c.newGP());
      c.mov(ptr, qword_ptr(stackTop));
      // return asBits & tagMask;
      c.and_(ptr, TaggedPointer<void *, 8>().pointerMask);

      // int64_t p;
      // memcpy(&p, s + instr->arg.l, sizeof(int64_t));
      // push(p);
      GPVar value(c.newGP());
      c.mov(value, qword_ptr(ptr, instr->arg.l));
      c.mov(qword_ptr(stackTop), value);

      c.unuse(ptr);
      c.unuse(value);
    } break;

    case STORE_ARY_ELM_INT: {
      c.comment("STORE_ARY_ELM_INT");

      // (get_memory<Variant*>(instr->arg.l))[1 + pop<int64_t>()] =
      // pop<int64_t>();
      GPVar index(c.newGP());
      c.mov(index, qword_ptr(stackTop));

      GPVar value(c.newGP());
      c.mov(value, qword_ptr(stackTop, -8));
      c.sub(stackTop, 16);

      GPVar array(c.newGP());
      c.mov(array, qword_ptr(memoryTop, (instr->arg.l * 8)));

      c.mov(qword_ptr(array, index, TIMES_8, 8), value);

      c.unuse(index);
      c.unuse(array);
      c.unuse(value);
    } break;
    case LOAD_ARY_ELM_INT: {
      c.comment("LOAD_ARY_ELM_INT");

      // push<int64_t>((get_memory<Variant*>(instr->arg.l))[1 +
      // pop<int64_t>()].l);
      GPVar index(c.newGP());
      c.mov(index, qword_ptr(stackTop));

      GPVar array(c.newGP());
      c.mov(array, qword_ptr(memoryTop, (instr->arg.l * 8)));

      GPVar tmp(c.newGP());
      c.mov(tmp, qword_ptr(array, index, TIMES_8, 8));
      c.mov(qword_ptr(stackTop), tmp);

      c.unuse(index);
      c.unuse(array);
      c.unuse(tmp);
    } break;

    case LOGICAL_AND: {
      c.comment("LOGICAL_AND");

      Label L_Neq = c.newLabel();
      Label L_after = c.newLabel();

      GPVar a(c.newGP());
      c.mov(a, qword_ptr(stackTop));
      c.sub(stackTop, 8);
      GPVar b(c.newGP());
      c.mov(b, qword_ptr(stackTop));
      // replace last value on stack with the ret

      c.cmp(a, 1);
      c.jne(L_Neq);
      c.cmp(b, 1);
      c.jne(L_Neq);

      c.mov(qword_ptr(stackTop), 1);
      c.jmp(L_after);

      c.bind(L_Neq);
      c.mov(qword_ptr(stackTop), 0);

      c.bind(L_after);

      c.unuse(a);
      c.unuse(b);
    } break;
    case LOGICAL_OR: {
      c.comment("LOGICAL_OR");

      Label L_Eq = c.newLabel();
      Label L_after = c.newLabel();

      GPVar a(c.newGP());
      c.mov(a, qword_ptr(stackTop));
      c.sub(stackTop, 8);
      GPVar b(c.newGP());
      c.mov(b, qword_ptr(stackTop));
      // replace last value on stack with the ret

      c.cmp(a, 1);
      c.je(L_Eq);
      c.cmp(b, 1);
      c.je(L_Eq);

      c.mov(qword_ptr(stackTop), 0);
      c.jmp(L_after);

      c.bind(L_Eq);
      c.mov(qword_ptr(stackTop), 1);

      c.bind(L_after);

      c.unuse(a);
      c.unuse(b);
    } break;

    default:
      raise_error("unimplemented opcode " + opcode_str[instr->op] + " in jit");
      break;
    }
  }

  c.bind(L_Return);

  c.mov(qword_ptr(stack), stackTop);

  c.ret();
  c.endFunction();

  this->native = function_cast<native_ptr>(c.make());
  if (!this->native)
    raise_error("unable to create jit'd block");

  if (start_from_ip == 0 && this->native) {
    for (auto *b : blocks)
      b->promote_call_to_native(
          program.get_block_ptr(program.get_block_id(this->get_name())));
  }

  // mark native segment as BASIC JIT compiled
  this->set_jit_type(BASIC);
}

void Block::optimizing_jit(const Program &program, unsigned int start_from_ip) {
  const auto &blocks = program.get_blocks();

  Compiler c;
  FileLogger logger(stderr);
  c.setLogger(&logger);

  // Tell compiler the function prototype we want. It allocates variables
  // representing
  // function arguments that can be accessed through Compiler or Function
  // instance.
  EFunction *func = c.newFunction(
      CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, int64_t *, int64_t *>());
  c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
  c.comment(
      std::string("optimized jit function '" + this->get_name() + "'").c_str());

  std::map<int, Label> label_positions;
  for (const auto &instr : instructions)
    if (instr.op == TJMP || instr.op == FJMP || instr.op == UJMP)
      label_positions[instr.arg.l] = c.newLabel();

  std::stack<BaseVar> stack;
  std::map<int, BaseVar> memory;

  int instr_count = 0;
  for (auto &elem : instructions) {
    auto label_position = label_positions.find(instr_count);
    if (label_position != label_positions.end())
      c.bind(label_position->second);

    ++instr_count;

    switch (elem.op) {
    case LIT_INT: {
      c.comment("LIT_INT");
      GPVar tmp(c.newGP());
      c.mov(tmp, elem.arg.l);

      stack.push(tmp);
    } break;

    case LOAD_INT:
    case LOAD_CHAR:
    case LOAD_FLOAT: {
      c.comment("LOAD_INT/CHAR/FLOAT");
      stack.push(memory[elem.arg.l]);
    } break;

    case STORE_INT:
    case STORE_CHAR:
    case STORE_FLOAT: {
      c.comment("STORE_INT/CHAR/FLOAT");

      GPVar tmp = static_cast<GPVar &>(stack.top());
      stack.pop();

      if (memory.find(elem.arg.l) != memory.end())
        c.unuse(memory[elem.arg.l]);

      memory[elem.arg.l] = tmp;
    } break;

    case LEQ_INT:
    case LE_INT:
    case GE_INT:
    case GEQ_INT:
    case EQ_INT:
    case NEQ_INT: {
      c.comment("CMP_INT");
      GPVar tmp1 = static_cast<GPVar &>(stack.top());
      stack.pop();
      GPVar tmp2 = static_cast<GPVar &>(stack.top());
      stack.pop();

      // branch
      Label L_LT = c.newLabel();
      Label L_Exit = c.newLabel();

      GPVar cond(c.newGP());
      c.cmp(tmp2, tmp1);

      switch (elem.op) {
      case LEQ_INT:
        c.jle(L_LT);
        break;
      case LE_INT:
        c.jl(L_LT);
        break;
      case GE_INT:
        c.jg(L_LT);
        break;
      case GEQ_INT:
        c.jge(L_LT);
        break;
      case EQ_INT:
        c.je(L_LT);
        break;
      case NEQ_INT:
        c.jne(L_LT);
        break;
      }

      c.mov(cond, 0);
      c.jmp(L_Exit);

      c.bind(L_LT);
      c.mov(cond, 1);
      c.bind(L_Exit);

      c.unuse(tmp1);
      c.unuse(tmp2);

      stack.push(cond);
    } break;

    case TJMP:
    case FJMP: {
      c.comment("FJMP/TJMP");
      GPVar tmp = static_cast<GPVar &>(stack.top());
      stack.pop();

      switch (elem.op) {
      case TJMP:
        c.cmp(tmp, 1);
        break;
      case FJMP:
        c.cmp(tmp, 0);
        break;
      }

      c.jz(label_positions[elem.arg.l]);
      c.unuse(tmp);
    } break;

    case ADD_INT:
    case SUB_INT:
    case MUL_INT: {
      c.comment("MATH_INT");
      GPVar a = static_cast<GPVar &>(stack.top());
      stack.pop();
      GPVar b = static_cast<GPVar &>(stack.top());
      stack.pop();

      switch (elem.op) {
      case ADD_INT:
        c.add(a, b);
        break;
      case SUB_INT:
        c.sub(a, b);
        break;
      case MUL_INT:
        c.imul(a, b);
        break;
      default:
        raise_error("failure");
        break;
      }

      stack.push(a);
      c.unuse(b);
    } break;

    case UJMP:
      c.comment("UJMP");
      c.jmp(label_positions[elem.arg.l]);
      break;

    case CALL:
      raise_error("'" + reinterpret_cast<Block *>(elem.arg.p)->get_name() +
                  "' must be jit'able to be called from a jit'd function");
      break;
    case CALL_NATIVE: {
      auto callee =
          std::find(blocks.begin(), blocks.end(), (Block *)elem.arg.p);
      if (callee == blocks.end())
        raise_error("couldn't identify block for jit native call");

      if ((*callee)->get_jit_type() != OPTIMIZING)
        raise_error(
            "all referenced functions must be jittabled and optimizing");

      if (!(*callee)->native && *callee != this)
        (*callee)->optimizing_jit(program);

      c.comment(
          std::string("CALL_NATIVE '" + (*callee)->get_name() + "'").c_str());

      GPVar s(c.argGP(0));
      GPVar m(c.argGP(1));

      if (this->get_memory_slots() > 0)
        c.add(qword_ptr(m), 8 * this->get_memory_slots());

      ECall *ctx = nullptr;
      if (*callee == this)
        ctx = c.call(func->getEntryLabel());
      else
        ctx = c.call(imm((sysint_t)(void *)(*callee)->native));

      GPVar ret(c.newGP());

      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder2<int64_t, long *, long *>());
      ctx->setArgument(0, s);
      ctx->setArgument(1, m);
      ctx->setReturn(ret);

      if ((*callee)->get_jit_type() == OPTIMIZING)
        stack.push(static_cast<BaseVar &>(ret));

      if (this->get_memory_slots() > 0)
        c.sub(qword_ptr(m), 8 * this->get_memory_slots());
    } break;

    case RTRN:
      // TODO fix floats
      {
        if (stack.size() >= 1) {
          BaseVar tmp = stack.top();
          stack.pop();
          c.ret(static_cast<GPVar &>(tmp));
        } else {
          c.ret();
        }
      }
      break;

    case LIT_LOAD_LE: {
      c.comment("LIT_LOAD_LE");

      GPVar tmp = static_cast<GPVar &>(memory[elem.arg.l]);
      GPVar ret(c.newGP());

      Label L_LE = c.newLabel();
      Label L_Exit = c.newLabel();

      c.cmp(tmp, elem.arg2.l);
      c.jl(L_LE);

      c.mov(ret, 0);
      c.jmp(L_Exit);

      c.bind(L_LE);
      c.mov(ret, 1);

      c.bind(L_Exit);

      stack.push(static_cast<BaseVar &>(ret));
    } break;

    case LIT_LOAD_ADD: {
      c.comment("LIT_LOAD_ADD");
      GPVar tmp = static_cast<GPVar &>(memory[elem.arg.l]);
      c.add(tmp, elem.arg2.l);
      stack.push(static_cast<BaseVar &>(tmp));
    } break;

    default:
      raise_error("unimplemented opcode " + opcode_str[elem.op] +
                  " in optimizing jit");
      break;
    }
  }

  c.endFunction();

  this->native = function_cast<native_ptr>(c.make());
  if (!this->native)
    raise_error("unable to create jit'd block");

  // mark native segment as OPTIMIZING JIT compiled
  this->set_jit_type(OPTIMIZING);
}

#endif //!NOJIT
