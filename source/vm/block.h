#ifndef BLOCK_H
#define BLOCK_H

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>
#include <list>
#include <map>
#if !NOJIT
#include <unordered_map>
#endif

#include "instruction.h"

class Memory;
class Program;

typedef int64_t (*native_ptr)(int64_t **, int64_t **);

class Block {
public:
  Block(const std::string &name);
  ~Block();
  const std::string &get_name() const;
  const Instruction *get_instruction(uint64_t index) const;
  const std::vector<Instruction> &get_instructions() const;
  size_t get_size() const;
  void add_instruction(const Instruction &instr);
  void calculate_memory_slots();
  void set_memory_slots(int64_t memory_slots);
  int64_t get_memory_slots() const { return this->memory_slots; }

#if !NOJIT
  unsigned int get_loop_hotness(const Instruction *i);
  void add_loop_hotness(const Instruction *i);
#endif

  // TODO, rename, repairs
  void absolute_jumps();
  void promote_call_to_native(const Block *target);

  // optimizations
  void fold_constants();
  void store_load_elimination();
  void inline_calls();

  // macro-opcodes
  void lit_load_add();
  void lit_load_sub();
  void lit_load_le();

  native_ptr native;

private:
  void calculate_int_fold(Instruction instr, std::list<Instruction> &calc_stack,
                          std::list<Instruction> &outputs);
  void calculate_float_fold(Instruction instr,
                            std::list<Instruction> &calc_stack,
                            std::list<Instruction> &outputs);
  void fold_ints();
  void fold_floats();

  std::string name;
  std::vector<Instruction> instructions;
  int64_t memory_slots;
#if !NOJIT
  std::unordered_map<const Instruction *, unsigned int> loop_hotness;
#endif

  friend std::ostream &operator<<(std::ostream &os, Block &b);
};

inline const Instruction *Block::get_instruction(uint64_t index) const {
  return &this->instructions[index];
}

#endif
