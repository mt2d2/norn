#include "memory.h"

#include <cstdlib>

#include "frame.h"

Memory::Memory() :
	allocated(std::vector<void*>())
{	
}

Memory::~Memory()
{
	// free any unfreed memory at end
	for (auto & elem : allocated)
		free(elem);
}


void* Memory::allocate(int64_t size)
{
	// do fancy gc things in here
	void* allocd = calloc(size, sizeof(uint8_t));
	this->allocated.push_back(allocd);
	
	return allocd;
}

Variant* Memory::new_lang_array(int size)
{
	size += 1;
	
	Variant* ret = reinterpret_cast<Variant*>(this->allocate(size * sizeof(Variant)));
	ret[0].l = size;

	return ret;
}
