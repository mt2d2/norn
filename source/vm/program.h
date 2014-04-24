#ifndef PROGRAM_H
#define PROGRAM_H

#include "block.h"

class Program
{
public:
	Program();
	void clean_up();
	Block* get_block_ptr(int key) const;
	void add_block(Block* block);
	int get_block_id(const std::string& key) const;
	int add_string(std::string string);
	std::string get_string(int key) const;
	void calculate_memory_slots();
	void set_memory_slots(int memory_slots);
	int get_memory_slots() const;
	const std::vector<Block*>& get_blocks() const;
	
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
	std::map<std::string, int> block_map;
	std::vector<Block*> blocks;
	std::vector<std::string> strings;
	int memory_slots;

	friend std::ostream& operator<<(std::ostream& os, Program& p)
	{
		for (std::vector<Block*>::iterator b = p.blocks.begin(); b != p.blocks.end(); ++b)
			std::cout << **b << std::endl;

		return os;
	}
};

inline Block* Program::get_block_ptr(int key) const
{
    return blocks[key];
}

#endif

