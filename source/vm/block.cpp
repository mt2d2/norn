#include "block.h"

#include "AsmJit/AsmJit.h"
using namespace AsmJit;	

Block::Block(const std::string& name) :
	native(NULL),
	name(name),
	needs_jit(false),
	instructions(std::vector<Instruction>()),
	memory_slots(0)
{
}

Block::~Block()
{
	if (this->native && this->get_needs_jit())
		MemoryManager::getGlobal()->free((void*)this->native);
}

std::string Block::get_name() const
{
    return this->name;
}

std::vector<Instruction>& Block::get_instructions()
{
	return this->instructions;
}

int Block::get_size()
{
	return this->instructions.size();
}

void Block::repair_disp_table(void** disp_table)
{
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
		i->op = reinterpret_cast<long>(disp_table[i->op]);
}

void Block::add_instruction(const Instruction& instruction)
{
	instructions.push_back(instruction);
}

void Block::calculate_memory_slots()
{
	int total = -1;
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
		if (i->op == STORE_INT || i->op == STORE_FLOAT || i->op == STORE_CHAR || i->op == STORE_ARY)
			if (i->arg.l > total)
				total = i->arg.l;
	
	if (total > -1)
		total += 1; /* 0-based */
	else 
		total = 0;
		
	this->set_memory_slots(total);
}

void Block::absolute_jumps()
{
	std::map<long, long> jmp_map;
	
	long instr_count = 0;
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
	{
		if (i->op == LBL)
		{
			jmp_map[i->arg.l] = instr_count;
			instructions.erase(i, i+1); // exclusive, i.e., upto
		}	
			
		++instr_count;
	}
	
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
		if (i->op == TJMP || i->op == FJMP || i->op == UJMP)
			i->arg.l = jmp_map[i->arg.l];
	
}

void Block::set_memory_slots(int memory_slots)
{
	this->memory_slots = memory_slots;
}

bool Block::get_needs_jit()
{
	return this->needs_jit;
}

void Block::set_needs_jit(bool needs_jit)
{
	this->needs_jit = needs_jit;
}
