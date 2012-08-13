#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "opcode.h"
#include "variant.h"

#include <iostream>

struct Instruction
{
	Instruction(Opcode op, const Variant& arg=Variant()) : op(op), arg(arg) { }

	long op;
	Variant arg;
	Variant arg2;
	// Variant arg3; // unused

	friend std::ostream& operator<<(std::ostream& os, const Instruction& i)
	{
		// return os << opcode_str[i.op] << " [i: " << i.arg.l << ", f: " << i.arg.d << "]";
		return os << opcode_str[i.op] << " " << i.arg.l;
	}
};

#endif
