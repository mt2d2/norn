#ifndef BLOCK_H
#define BLOCK_H

#include "instruction.h"
#include "variant.h"

#include <iostream>
#include <string>
#include <vector>
#include <list>

enum JITType
{
	NONE,
	BASIC,
	OPTIMIZING
};

typedef long(*native_ptr)(long**, long**);

class Block
{
public:
	Block(const std::string& name);
	~Block();
	const std::string& get_name() const;
	const Instruction* get_instruction(int index) const;
	const std::vector<Instruction>& get_instructions() const;
	int get_size() const;
	void add_instruction(const Instruction& instr);
	void calculate_memory_slots();
	void set_memory_slots(int memory_slots);
	int get_memory_slots() const
	{
		return this->memory_slots;
	}
	JITType get_jit_type() const;
	void set_jit_type(JITType needs_jit);

	// TODO, rename, repairs
	void repair_disp_table(void** disp_table);
	void absolute_jumps();

	// optimizations
	void fold_constants();
	void store_load_elimination();
	void inline_calls();

	// macro-opcodes
	void lit_load_add();
	void lit_load_sub();
	void lit_load_le();
	
	// jit
	void jit(std::vector<Block*>& blocks);
	void optimizing_jit(std::vector<Block*>& blocks);

	native_ptr native;

private:
	void calculate_int_fold(Instruction instr, std::list<Instruction>& calc_stack, std::list<Instruction>& outputs);
	void calculate_float_fold(Instruction instr, std::list<Instruction>& calc_stack, std::list<Instruction>& outputs);
	void fold_ints();
	void fold_floats();
	
	std::string name;
	JITType jit_type;
	std::vector<Instruction> instructions;
	int memory_slots;

	friend std::ostream& operator<<(std::ostream& os, Block& b)
	{
		os << "Name: " << b.name << "; Memory Slots: " << b.get_memory_slots() << std::endl;
		os << b.name << std::endl;

		for (const auto& i : b.instructions)
			os << i << std::endl;

		return os;
	}
};

inline const Instruction* Block::get_instruction(int index) const
{
    return &this->instructions[index];
}

#endif
