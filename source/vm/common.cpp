#include "common.h"

#include <string>
#include <iostream>
#include <cstdlib> // exit

#include <execinfo.h>
#include <cstdlib>

/* Obtain a backtrace and print it to stdout. */
void print_trace()
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);

	for (i = 0; i < size; i++)
		std::cerr << strings[i] << std::endl;

	free(strings);
}

void raise_error(const std::string& message)
{
	std::cerr << message << std::endl;
	print_trace();
    exit(1);
}
