#ifndef MEMORY_H
#define MEMORY_H

#include <vector>

#include "variant.h"

class Memory
{
public:
	Memory();
	~Memory();

	Variant* new_lang_array(int size);	
	void* allocate(int64_t size);

private:	
	std::vector<void*> allocated;
	
	// long** stack;
	// long** memory;
};

#endif // MEMORY_H
