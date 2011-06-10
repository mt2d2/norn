#ifndef BLOCK_H
#define BLOCK_H

#include "instruction.h"
#include "variant.h"
#include "../type.h"

#include <iostream>
#include <string>
#include <vector>
#include <list>

typedef void(*native_ptr)(long**, long**);

class Block
{
public:
	Block(const std::string& name);
	~Block();
	std::string get_name() const;
	Instruction* get_instruction(int index);
	std::vector<Instruction>& get_instructions();
	int get_size();
	void add_instruction(const Instruction& instr);
	void calculate_memory_slots();
	void set_memory_slots(int memory_slots);
	int get_memory_slots() const
	{
		return this->memory_slots;
	}
	bool get_needs_jit();
	void set_needs_jit(bool needs_jit);

	// TODO, rename, repairs
	void repair_disp_table(void** disp_table);
	void absolute_jumps();

	// optimizations
	void calculate_int_fold(Instruction instr, std::list<Instruction>& calc_stack, std::list<Instruction>& outputs);
	void fold_constants();
	void store_load_elimination();
	void inline_calls();

	// macro-opcodes
	void lit_load_add();
	void lit_load_sub();
	void lit_load_le();
	
	// jit
	void jit(std::vector<Block*>& blocks);

	native_ptr native;

private:
	std::string name;
	bool needs_jit;
	std::vector<Instruction> instructions;
	int memory_slots;

	friend std::ostream& operator<<(std::ostream& os, Block& b)
	{
		os << "Name: " << b.name << "; Memory Slots: " << b.get_memory_slots() << std::endl;

		for (std::vector<Instruction>::iterator i = b.instructions.begin(); i != b.instructions.end(); ++i)
			os << *i << std::endl;

		return os;
	}
};

inline Instruction* Block::get_instruction(int index)
{
    return &this->instructions[index];
}

#endif
