#ifndef MEMORY_H
#define MEMORY_H

#include <vector>

#include "variant.h"

class Memory
{
public:
	Memory();
	~Memory();
	
	template<typename T> 
	T* new_root_array(int size);
	
	Variant* new_lang_array(int size);	

private:
	template<typename T>
	T* allocate(int size);
	
	std::vector<void*> allocated;
	
	long** stack;
	long** memory;
};

#endif // MEMORY_H
