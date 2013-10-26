#include "program.h"

#include "block.h"
#include "common.h"

#include <cmath> // sqrt
#include <cstring> // memcpy

Program::Program() : 
	block_map(std::map<std::string, int>()), 
	blocks(std::vector<Block*>()), 
	strings(std::vector<std::string>()),
	memory_slots(0)
{
}

void Program::clean_up()
{
	for (const auto* b : blocks)
		delete b;
}

void Program::add_block(Block* block)
{
	std::map<std::string, int>::iterator it = block_map.find(block->get_name());
 	if (it == block_map.end())
    {
		// not found
		block_map[block->get_name()] = blocks.size();
		blocks.push_back(block);
    }
    else
    {
        blocks[block_map[block->get_name()]] = block;
    }
}

int Program::get_block_id(const std::string& key)
{
	if (block_map.find(key) == block_map.end())
		raise_error("undefined function: " + key);

    return block_map[key];
}

int Program::add_string(std::string string)
{
	int ret = strings.size();
	strings.push_back(string);
	return ret;
}

std::string Program::get_string(int key) const
{
	return strings[key];
}

void Program::calculate_memory_slots()
{
	int total = 0;	
	for (auto* b : blocks)
	{
		b->calculate_memory_slots();
		total += b->get_memory_slots();
	}
	
	this->set_memory_slots(total);
}

void Program::set_memory_slots(int memory_slots)
{
	this->memory_slots = memory_slots;
}

int Program::get_memory_slots() const
{
	return this->memory_slots;
}

void Program::repair_disp_table(void** disp_table)
{
	for (auto* b : blocks)
		b->repair_disp_table(disp_table);
}

void Program::absolute_jumps()
{
	for (auto* b : blocks)
		b->absolute_jumps();
}

void Program::store_load_elimination()
{
	for (auto* b : blocks)
		b->store_load_elimination();
}

void Program::fold_constants()
{
	for (auto* b : blocks)
		b->fold_constants();
}

void Program::inline_calls()
{
	for (auto* b : blocks)
		b->inline_calls();
}

void Program::lit_load_add()
{
	for (auto* b : blocks)
		b->lit_load_add();
}

void Program::lit_load_sub()
{
	for (auto* b : blocks)
		b->lit_load_sub();
}

void Program::lit_load_le()
{
	for (auto* b : blocks)
		b->lit_load_le();
}

void Program::jit()
{
	for (auto* b : blocks)
	{
		if (!b->native) 
		{
			if (b->get_jit_type() == BASIC)
				b->jit(this->blocks);
			else if (b->get_jit_type() == OPTIMIZING)
				b->optimizing_jit(this->blocks);
		}
	}
}