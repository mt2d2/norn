#include "common.h"

#include <string>
#include <iostream>
#include <cstdlib> // exit

void raise_error(const std::string& message)
{
	std::cout << message << std::endl;
    exit(1);
}
