#ifndef BLOCK_H
#define BLOCK_H

#include "instruction.h"
#include "variant.h"

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>

enum JITType
{
	NONE,
	BASIC,
	OPTIMIZING
};

typedef int64_t(*native_ptr)(int64_t**, int64_t**);

class Program;

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

#if !NOJIT
	JITType get_jit_type() const;
	void set_jit_type(JITType needs_jit);
	unsigned int get_hotness() const;
	void add_hotness();
	unsigned int get_backedge_hotness(const Instruction* i) const;
	void add_backedge_hotness(const Instruction* i);
#endif

	// TODO, rename, repairs
	void absolute_jumps();
	void promote_call_to_native(const Block* target);

	// optimizations
	void fold_constants();
	void store_load_elimination();
	void inline_calls();

	// macro-opcodes
	void lit_load_add();
	void lit_load_sub();
	void lit_load_le();
	
	// jit
	void jit(const Program& blocks, unsigned int start_from_ip=0);
	void optimizing_jit(const Program& program, unsigned int start_from_ip=0);

	native_ptr native;

private:
	void calculate_int_fold(Instruction instr, std::list<Instruction>& calc_stack, std::list<Instruction>& outputs);
	void calculate_float_fold(Instruction instr, std::list<Instruction>& calc_stack, std::list<Instruction>& outputs);
	void fold_ints();
	void fold_floats();
	
	std::string name;
	std::vector<Instruction> instructions;
	unsigned int memory_slots;
#if !NOJIT
	JITType jit_type;
	unsigned int hotness;
	std::map<const Instruction*, unsigned int> backedge_hotness;
#endif
	
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
