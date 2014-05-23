#include "block.h"

#if !NOJIT
#include "AsmJit/AsmJit.h"
using namespace AsmJit;
#endif

Block::Block(const std::string &name)
    : native(nullptr), name(name), instructions(std::vector<Instruction>()),
      memory_slots(0)
#if !NOJIT
      ,
      jit_type(NONE), hotness(0),
      backedge_hotness(std::map<const Instruction *, unsigned int>())
#endif
{
}

Block::~Block() {
#if !NOJIT
  this->free_native_code();
#endif
}

const std::string &Block::get_name() const { return this->name; }

const std::vector<Instruction> &Block::get_instructions() const {
  return this->instructions;
}

int Block::get_size() const { return this->instructions.size(); }

#if !NOJIT
void Block::free_native_code() {
  if (this->native &&
      (this->get_jit_type() == BASIC || this->get_jit_type() == OPTIMIZING))
    MemoryManager::getGlobal()->free((void *)this->native);
}

JITType Block::get_jit_type() const { return this->jit_type; }

void Block::set_jit_type(JITType jit_type) { this->jit_type = jit_type; }

unsigned int Block::get_hotness() const { return this->hotness; }

void Block::add_hotness() { this->hotness += 1; }

unsigned int Block::get_backedge_hotness(const Instruction *i) const {
  return this->backedge_hotness.at(i);
}

void Block::add_backedge_hotness(const Instruction *i) {
  this->backedge_hotness[i]++;
}
#endif

void Block::add_instruction(const Instruction &instruction) {
  instructions.push_back(instruction);
}

void Block::calculate_memory_slots() {
  int total = -1;
  for (const auto &i : instructions)
    if (i.op == STORE_INT || i.op == STORE_FLOAT || i.op == STORE_CHAR ||
        i.op == STORE_ARY)
      if (i.arg.l > total)
        total = i.arg.l;

  if (total > -1)
    total += 1; /* 0-based */
  else
    total = 0;

  this->set_memory_slots(total);
}

void Block::absolute_jumps() {
  std::map<long, long> jmp_map;
  long instr_count = 0;

  for (auto i = instructions.begin(); i != instructions.end(); ++i) {
    if (i->op == LBL) {
      jmp_map[i->arg.l] = instr_count;
      instructions.erase(i, i + 1); // exclusive, i.e., upto
    }

    ++instr_count;
  }

  for (auto &i : instructions)
    if (i.op == TJMP || i.op == FJMP || i.op == UJMP)
      i.arg.l = jmp_map[i.arg.l];
}

void Block::promote_call_to_native(const Block *target) {
  for (auto &i : instructions)
    if (i.op == CALL && reinterpret_cast<Block *>(i.arg.p) == target)
      i.op = CALL_NATIVE;
}

void Block::set_memory_slots(int memory_slots) {
  this->memory_slots = memory_slots;
}
