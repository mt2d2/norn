#ifndef MACHINE_H
#define MACHINE_H

#include <cstdio> // printf
#include <cstring> // memcpy

#include "block.h"
#include "common.h"
#include "frame.h"
#include "instruction.h"
#include "memory.h"
#include "program.h"
#include "variant.h"

#define STACK_SIZE 256

class Machine
{
public:
	Machine(const Program& program, bool debug=false, bool nojit=false);
	~Machine();
	void execute();

private:
	template<typename T> T get_memory(int key);
	template<typename T> void store_memory(int key, T value);
	
	bool frames_is_empty();
	void push_frame(const Frame& bookmark);
	Frame& pop_frame();
	
	bool stack_is_empty();
	template<typename T> void push(T element);
	template<typename T> T pop();

	Program program;
	Block* block;
	const Instruction* instr;
	Memory manager;
	int ip;
	int ipc;
	bool debug;
	bool nojit;

	long* stack;
	long* stack_start;
	Frame* frames;
	Frame* frames_start;
	long* memory;
	
};

template<typename T>
inline T Machine::get_memory(int key)
{
	return (T)memory[key];
}

template<>
inline double Machine::get_memory(int key)
{
	double ret;
	memcpy(&ret, &memory[key], sizeof(double));
	return ret;
}

template<typename T>
inline void Machine::store_memory(int key, T value)
{
	memory[key] = (long)value;
}

template<>
inline void Machine::store_memory(int key, double value)
{
	memcpy(&memory[key], &value, sizeof(double));
}

inline bool Machine::frames_is_empty()
{
	return (frames == frames_start);
}

inline void Machine::push_frame(const Frame& bookmark)
{
	*(++frames) = bookmark;
}

inline Frame& Machine::pop_frame()
{
	return *(frames--);
}

inline bool Machine::stack_is_empty()
{
	return (stack == stack_start);
}

template<typename T>
inline void Machine::push(T element)
{
	*++stack = (long)element;
}

template<>
inline void Machine::push(double element)
{
	long tmp;
	memcpy(&tmp, &element, sizeof(double));
	*++stack = tmp;
}

template<typename T>
inline T Machine::pop()
{
	return (T)*stack--;
}

template<>
inline double Machine::pop()
{
	double ret;
	memcpy(&ret, stack--, sizeof(double));
	return ret;
}

#endif
