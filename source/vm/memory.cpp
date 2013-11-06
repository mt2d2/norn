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


template<typename T>
T* Memory::allocate(int size)
{
	// do fancy gc things in here
	void* allocd = calloc(size, sizeof(T));
	this->allocated.push_back(allocd);
	
	return reinterpret_cast<T*>(allocd);
}

template<typename T> 
T* Memory::new_root_array(int size) 
{ 
	return this->allocate<T>(size);
}

template long* Memory::new_root_array(int size);
template Frame* Memory::new_root_array(int size);

Variant* Memory::new_lang_array(int size)
{
	size += 1;
	
	Variant* ret = this->allocate<Variant>(size);
	ret[0].l = size;

	return ret;
}
