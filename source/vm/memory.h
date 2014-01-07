#ifndef MEMORY_H
#define MEMORY_H

#include <unordered_set>

#include "variant.h"


struct AllocatedMemory
{
	bool marked;
	void *data;

	AllocatedMemory(uint64_t size);
	~AllocatedMemory();
};

class Memory
{
public:
	Memory(int64_t* stack_start, int64_t* memory_start);
	~Memory();

	Variant* new_lang_array(int size);	
	AllocatedMemory* allocate(int64_t size);

	void set_stack(int64_t* stack);
	void set_memory(int64_t* memory);

private:	
	std::unordered_set<AllocatedMemory*> allocated;

	bool is_managed(AllocatedMemory *memory);
	void mark();
	void sweep();
	void gc();
	
	int64_t* stack_start;
	int64_t* stack;
	int64_t* memory_start;
	int64_t* memory;
};

#endif // MEMORY_H
