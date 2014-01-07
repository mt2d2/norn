#include "memory.h"

#include "frame.h"
#include "common.h"

#include <dlmalloc.h>

#define GC_THRESHOLD 100

AllocatedMemory::AllocatedMemory(uint64_t size)
{
	this->marked = false;
	this->data = dlmalloc(size);
}

AllocatedMemory::~AllocatedMemory()
{
	dlfree(this->data);
}

Memory::Memory(int64_t* stack_start, int64_t* memory_start) :
	allocated(std::unordered_set<AllocatedMemory*>()),
	stack_start(stack_start),
	stack(nullptr),
	memory_start(memory_start),
	memory(nullptr)
{	
}

Memory::~Memory()
{
	// free any unfreed memory at end
	for (auto *elem : allocated)
		delete elem;
}

AllocatedMemory* Memory::allocate(int64_t size)
{
	if (allocated.size() == GC_THRESHOLD)
		gc();

	auto ret = this->allocated.emplace(new AllocatedMemory(size));

	if (!ret.second)
		raise_error("allocation failure, not unique");

	return *ret.first;
}

void Memory::set_stack(int64_t* stack)
{
	this->stack = stack;
}

void Memory::set_memory(int64_t* memory)
{
	this->memory = memory;
}

Variant* Memory::new_lang_array(int size)
{
	size += 1;
	
	// Variant* ret = reinterpret_cast<Variant*>(this->allocate(size * sizeof(Variant)).data);
	Variant* ret = new Variant[size];
	ret[0].l = size;

	return ret;
}

bool Memory::is_managed(AllocatedMemory *memory)
{
	return allocated.find(memory) != allocated.end();
}

void Memory::mark()
{
	// check stack
	while (stack != stack_start)
	{
		if (is_managed(reinterpret_cast<AllocatedMemory*>(*stack)))
			reinterpret_cast<AllocatedMemory*>(*stack)->marked = true;
		stack--;
	}

	// check memory
	while (memory != memory_start)
	{
		if (is_managed(reinterpret_cast<AllocatedMemory*>(*memory)))
			reinterpret_cast<AllocatedMemory*>(*memory)->marked = true;
		memory--;
	}
}

void Memory::sweep()
{
	for (auto it = allocated.begin(); it != allocated.end();)
	{
		auto alloc = *it;

		if (!alloc->marked)
		{
			delete alloc;
			it = allocated.erase(it);
		}
		else
		{
			alloc->marked = false;
			++it;
		}
	}
}

void Memory::gc()
{
	// std::cout << "GC" << std::endl;
	// auto start = allocated.size();

	mark();
	sweep();

	// auto finish = allocated.size();
	// fprintf(stderr, "swept %lu objects, %lu remaining\n", start - finish, finish);

	dlmalloc_trim(0);
	// dlmalloc_stats();
}