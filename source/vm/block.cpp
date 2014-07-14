#include "block.h"

Block::Block(const std::string &name)
    : name(name), instructions(std::vector<Instruction>()), memory_slots(0)
#if !NOJIT
      ,
      loop_hotness(std::unordered_map<const Instruction *, unsigned int>())
#endif
{
  for (const auto &i : instructions) {
    loop_hotness[&i] = 0;
  }
}

Block::~Block() {}

const std::string &Block::get_name() const { return this->name; }

const std::vector<Instruction> &Block::get_instructions() const {
  return this->instructions;
}

int Block::get_size() const { return this->instructions.size(); }

#if !NOJIT
unsigned int Block::get_loop_hotness(const Instruction *i) {
  return this->loop_hotness[i];
}

void Block::add_loop_hotness(const Instruction *i) { this->loop_hotness[i]++; }
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

  auto i = instructions.begin();
  while (i != instructions.end()) {
    if (i->op == LBL) {
      jmp_map[i->arg.l] = instr_count;
      i = instructions.erase(i, i + 1); // exclusive, i.e., upto
    } else {
      ++i;
      ++instr_count;
    }
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
