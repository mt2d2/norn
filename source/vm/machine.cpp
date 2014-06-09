#include "machine.h"
#include <cmath>  // fmod
#include <cstdio> // printf

#define LOOP_HOTNESS 56

#define COMPUTED_GOTO __GNUC__
#if COMPUTED_GOTO
#define DISPATCH NEXT
#define OP(x)                                                                  \
  trace_##x : { trace.record(instr); }                                         \
  x:

#define NEXT                                                                   \
  instr = block->get_instruction(ip++);                                        \
  goto *disp_table[instr->op];
#define END_DISPATCH
#else
#define DISPATCH                                                               \
  while (true) {                                                               \
    instr = block->get_instruction(ip++);                                      \
    std::cout << *instr << std::endl;                                          \
    switch (instr->op) {
#define OP(x)                                                                  \
  {                                                                            \
  case x:
#define NEXT break;
#define END_DISPATCH                                                           \
  }                                                                            \
  }
#endif

// TODO test perf of switch root_arrays to vectors, would make a lot
// simpler!
// TODO refactor Opcode so reapir_disp_table isn't needed
// TODO reference things by frame, push inital frame for main, too
Machine::Machine(const Program &program
#if !NOJIT
                 ,
                 bool debug, bool nojit
#endif
                 )
    : program(program), block(nullptr), instr(nullptr), ip(0),
#if !NOJIT
      debug(debug), nojit(nojit),
#endif
      stack(new int64_t[STACK_SIZE]), stack_start(stack),
      frames(new Frame[STACK_SIZE]), frames_start(frames),
      memory(new int64_t[STACK_SIZE * this->program.get_memory_slots()]),
      manager(Memory(stack_start, memory)), trace(Trace()), is_tracing(false) {
}

Machine::~Machine() {
  delete[] stack_start;
  delete[] frames_start;
  delete[] memory;
}

void Machine::execute() {

#if COMPUTED_GOTO
#include "goto.h"
  void **disp_table = op_disp_table;
#endif

  block = this->program.get_block_ptr(this->program.get_block_id("main"));

  DISPATCH

  OP(LIT_INT) {
    push<int64_t>(instr->arg.l);
    NEXT
  }

  OP(LIT_FLOAT) {
    push<double>(instr->arg.d);
    NEXT
  }

  OP(LIT_CHAR) {
    push<char>(instr->arg.c);
    NEXT
  }

  OP(LOAD_INT) {
    push<int64_t>(get_memory<int64_t>(instr->arg.l));
    NEXT
  }

  OP(LOAD_FLOAT) {
    push<double>(get_memory<double>(instr->arg.l));
    NEXT
  }

  OP(LOAD_CHAR) {
    push<char>(get_memory<char>(instr->arg.l));
    NEXT
  }

  OP(STORE_INT) {
    store_memory(instr->arg.l, pop<int64_t>());
    NEXT
  }

  OP(STORE_FLOAT) {
    store_memory(instr->arg.l, pop<double>());
    NEXT
  }

  OP(STORE_CHAR) {
    store_memory(instr->arg.l, pop<char>());
    NEXT
  }

  OP(ADD_INT) {
    push<int64_t>(pop<int64_t>() + pop<int64_t>());
    NEXT
  }

  OP(ADD_FLOAT) {
    push<double>(pop<double>() + pop<double>());
    NEXT
  }

  OP(SUB_INT) {
    push<int64_t>(pop<int64_t>() - pop<int64_t>());
    NEXT
  }

  OP(SUB_FLOAT) {
    push<double>(pop<double>() - pop<double>());
    NEXT
  }

  OP(MUL_INT) {
    push<int64_t>(pop<int64_t>() * pop<int64_t>());
    NEXT
  }

  OP(MUL_FLOAT) {
    push<double>(pop<double>() * pop<double>());
    NEXT
  }

  OP(DIV_INT) {
    push<int64_t>(pop<int64_t>() / pop<int64_t>());
    NEXT
  }

  OP(DIV_FLOAT) {
    push<double>(pop<double>() / pop<double>());
    NEXT
  }

  OP(MOD_INT) {
    push<int64_t>(pop<int64_t>() % pop<int64_t>());
    NEXT
  }

  OP(MOD_FLOAT) {
    push<double>(fmod(pop<double>(), pop<double>()));
    NEXT
  }

  OP(AND_INT) {
    push<int64_t>(pop<int64_t>() & pop<int64_t>());
    NEXT
  }

  OP(OR_INT) {
    push<int64_t>(pop<int64_t>() | pop<int64_t>());
    NEXT
  }

  OP(LE_INT) {
    push<bool>(pop<int64_t>() < pop<int64_t>());
    NEXT
  }

  OP(LE_FLOAT) {
    push<bool>(pop<double>() < pop<double>());
    NEXT
  }

  OP(LEQ_INT) {
    push<bool>(pop<int64_t>() <= pop<int64_t>());
    NEXT
  }

  OP(LEQ_FLOAT) {
    push<bool>(pop<double>() <= pop<double>());
    NEXT
  }

  OP(GE_INT) {
    push<bool>(pop<int64_t>() > pop<int64_t>());
    NEXT
  }

  OP(GE_FLOAT) {
    push<bool>(pop<double>() > pop<double>());
    NEXT
  }

  OP(GEQ_INT) {
    push<bool>(pop<int64_t>() >= pop<int64_t>());
    NEXT
  }

  OP(GEQ_FLOAT) {
    push<bool>(pop<double>() >= pop<double>());
    NEXT
  }

  OP(EQ_INT) {
    push<bool>(pop<int64_t>() == pop<int64_t>());
    NEXT
  }

  OP(EQ_FLOAT) {
    push<bool>(pop<double>() == pop<double>());
    NEXT
  }

  OP(NEQ_INT) {
    push<bool>(pop<int64_t>() != pop<int64_t>());
    NEXT
  }

  OP(NEQ_FLOAT) {
    push<bool>(pop<double>() != pop<double>());
    NEXT
  }

  OP(TJMP) {
    if (pop<bool>())
      ip = instr->arg.l;
    NEXT
  }

  OP(FJMP) {
    if (!pop<bool>())
      ip = instr->arg.l;
    NEXT
  }

  OP(UJMP) {
    ip = instr->arg.l;
    NEXT
  }

  OP(PRINT_INT) {
    printf("%lld", pop<int64_t>());
    NEXT
  }

  OP(PRINT_FLOAT) {
    printf("%f", pop<double>());
    NEXT
  }

  OP(PRINT_CHAR) {
    putchar(pop<char>());
    NEXT
  }

  OP(NEW_ARY) {
    push<Variant *>(manager.new_lang_array(instr->arg.l));
    NEXT
  }

  OP(STORE_ARY) {
    store_memory(instr->arg.l, pop<Variant *>());
    NEXT
  }

  OP(LOAD_ARY) {
    push<Variant *>(get_memory<Variant *>(instr->arg.l));
    NEXT
  }

  OP(MALLOC) {
    manager.set_stack(stack);
    manager.set_memory(memory + block->get_memory_slots());
    push(reinterpret_cast<uint8_t *>(manager.allocate(pop<int64_t>())));
    NEXT
  }

  OP(STRUCT_STORE_INT) {
    auto s = reinterpret_cast<uint8_t *>(
        pop<AllocatedMemory *>()->data.getPointer());
    auto v = pop<int64_t>();
    memcpy(s + instr->arg.l, &v, sizeof(int64_t));

    NEXT
  }

  OP(STRUCT_STORE_FLOAT) {
    auto s = reinterpret_cast<uint8_t *>(
        pop<AllocatedMemory *>()->data.getPointer());
    auto v = pop<double>();
    memcpy(s + instr->arg.l, &v, sizeof(double));
    NEXT
  }

  OP(STRUCT_STORE_CHAR) {
    auto s = reinterpret_cast<uint8_t *>(
        pop<AllocatedMemory *>()->data.getPointer());
    auto v = pop<char>();
    memcpy(s + instr->arg.l, &v, sizeof(char));
    NEXT
  }

  OP(STRUCT_LOAD_INT) {
    auto s = reinterpret_cast<uint8_t *>(
        pop<AllocatedMemory *>()->data.getPointer());
    int64_t p;
    memcpy(&p, s + instr->arg.l, sizeof(int64_t));
    push(p);
    NEXT
  }

  OP(STRUCT_LOAD_FLOAT) {
    auto s = reinterpret_cast<uint8_t *>(
        pop<AllocatedMemory *>()->data.getPointer());
    double p;
    memcpy(&p, s + instr->arg.l, sizeof(double));
    push(p);
    NEXT
  }

  OP(STRUCT_LOAD_CHAR) {
    auto s = reinterpret_cast<uint8_t *>(
        pop<AllocatedMemory *>()->data.getPointer());
    char p;
    memcpy(&p, s + instr->arg.l, sizeof(char));
    push(p);
    NEXT
  }

  OP(CPY_ARY_CHAR) {
    Variant *array = pop<Variant *>();
    for (int64_t i = 1; i < array[0].l; ++i)
      array[i].c = program.get_string(instr->arg.l)[i - 1];

    push<Variant *>(array);
    NEXT
  }

  OP(PRINT_ARY_CHAR) {
    Variant *array = pop<Variant *>();
    for (int64_t i = 1; i < array[0].l; ++i)
      putchar(array[i].c);
    NEXT
  }

  OP(CALL) {
    memory += block->get_memory_slots();
    push_frame(Frame(ip, block));
    block = reinterpret_cast<Block *>(instr->arg.p);
    ip = 0;
    NEXT
  }

  OP(CALL_NATIVE) { raise_error("CALL_NATIVE disabled"); }

  OP(RTRN) {
    if (likely(!frames_is_empty())) {
      Frame f = pop_frame();
      block = f.block;
      ip = f.ip;
      memory -= block->get_memory_slots();
    } else {
      // clean and final exit
      goto cleanup;
    }
    NEXT
  }

  OP(STORE_ARY_ELM_CHAR) {
    (get_memory<Variant *>(instr->arg.l))[1 + pop<int64_t>()] = pop<int64_t>();
    NEXT
  }

  OP(STORE_ARY_ELM_INT) {
    (get_memory<Variant *>(instr->arg.l))[1 + pop<int64_t>()] = pop<int64_t>();
    NEXT
  }

  OP(STORE_ARY_ELM_FLOAT) {
    (get_memory<Variant *>(instr->arg.l))[1 + pop<int64_t>()] = pop<double>();
    NEXT
  }

  OP(LOAD_ARY_ELM_CHAR) {
    push<char>((get_memory<Variant *>(instr->arg.l))[1 + pop<int64_t>()].c);
    NEXT
  }

  OP(LOAD_ARY_ELM_INT) {
    push<int64_t>((get_memory<Variant *>(instr->arg.l))[1 + pop<int64_t>()].l);
    NEXT
  }

  OP(LOAD_ARY_ELM_FLOAT) {
    push<double>((get_memory<Variant *>(instr->arg.l))[1 + pop<int64_t>()].d);
    NEXT
  }

  OP(LIT_LOAD_ADD) {
    push<int64_t>(get_memory<int64_t>(instr->arg.l) + instr->arg2.l);
    NEXT
  }

  OP(LIT_LOAD_SUB) {
    push<int64_t>(get_memory<int64_t>(instr->arg.l) - instr->arg2.l);
    NEXT
  }

  OP(LIT_LOAD_LE) {
    push<int64_t>(get_memory<int64_t>(instr->arg.l) < instr->arg2.l);
    NEXT
  }

  OP(F2I) {
    push<int64_t>(pop<double>());
    NEXT
  }

  OP(I2F) {
    push<double>(pop<int64_t>());
    NEXT
  }

  OP(LBL) {
    raise_error("fatal label encounter in virtual machine");
    NEXT
  }

  OP(LOOP) {
    if (!nojit) {
      if (is_tracing && trace.is_head(instr)) {
        disp_table = op_disp_table;
        is_tracing = false;

        if (debug) {
          printf("trace finished\n");
          trace.debug();
          trace.jit();
        }

        NEXT
      }

      block->add_loop_hotness(instr);

      if (block->get_loop_hotness(instr) == LOOP_HOTNESS) {
        if (debug) {
          printf("trace started\n");
        }

        // disp_table = trace_disp_table
        disp_table = trace_disp_table;
        is_tracing = true;

        // hack to record first LOOP
        trace.record(instr);
      }
    }

    NEXT
  }

  OP(LOGICAL_AND) {
    push<bool>(pop<bool>() && pop<bool>());
    NEXT
  }

  OP(LOGICAL_OR) {
    push<bool>(pop<bool>() || pop<bool>());
    NEXT
  }

  END_DISPATCH

cleanup:
  program.clean_up();

  return;
}
