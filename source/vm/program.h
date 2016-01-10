#ifndef PROGRAM_H
#define PROGRAM_H

#include <cstdint>
#include <iosfwd>

#include "block.h"

class Program {
public:
  Program();
  void clean_up();
  Block *get_block_ptr(int64_t key) const;
  void add_block(Block *block);
  int64_t get_block_id(const std::string &key) const;
  int64_t add_string(std::string string);
  std::string get_string(int64_t key) const;
  void calculate_memory_slots();
  void set_memory_slots(int64_t memory_slots);
  int64_t get_memory_slots() const;
  const std::vector<Block *> &get_blocks() const;

  // repairs
  void absolute_jumps();

  // optimizations
  void store_load_elimination();
  void fold_constants();
  void inline_calls();

  // macro opcodes
  void lit_load_add();
  void lit_load_sub();
  void lit_load_le();

private:
  std::map<std::string, int64_t> block_map;
  std::vector<Block *> blocks;
  std::vector<std::string> strings;
  int64_t memory_slots;

  friend std::ostream &operator<<(std::ostream &os, Program &p);
};

inline Block *Program::get_block_ptr(int64_t key) const { return blocks[key]; }

extern "C" void Program_copy_array_char(Program *program, int key,
                                        Variant *array);
extern "C" void Program_print_array_char(Variant *array);

#endif
