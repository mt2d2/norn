#include "machine.h"
#include <cmath> // fmod
#include <cstdio> // printf

#define COMPUTED_GOTO __GNUC__
#if COMPUTED_GOTO
#	define DISPATCH NEXT
#	define OP(x) x:
#	define NEXT /*++ipc;*/ instr = block->get_instruction(ip++); goto *(reinterpret_cast<void*>(instr->op));
#	define END_DISPATCH
#else
#	define DISPATCH while (true) { ++ipc; instr = block->get_instruction(ip++); /*std::cout << *instr << std::endl;*/ switch(instr->op) {
#	define OP(x) case x:
#	define NEXT break;
#	define END_DISPATCH } }
#endif

// TODO test perf of switch root_arrays to vectors, would make a lot simpler!
// TODO refactor Opcode so reapir_disp_table isn't needed
// TODO reference things by frame, push inital frame for main, too
Machine::Machine(const Program& program, bool debug) :
	program(program),
	block(NULL),
	instr(NULL),
	manager(Memory()),
	ip(0),
	ipc(0),
	debug(debug),
	stack(NULL),
	stack_start(NULL),	
	frames(NULL),
	frames_start(NULL),
	memory(NULL)
{
}

void Machine::execute()
{
	this->program.jit();
	
#if COMPUTED_GOTO
#	include "goto.h"
	program.repair_disp_table(disp_table);
#endif
		
	this->stack = reinterpret_cast<long*>(alloca(sizeof(long) * STACK_SIZE));
	this->stack_start = this->stack;
	this->frames = reinterpret_cast<Frame*>(alloca(sizeof(Frame) * STACK_SIZE));
	this->frames_start = this->frames;
	this->memory = reinterpret_cast<long*>(alloca(sizeof(long) * STACK_SIZE * this->program.get_memory_slots()));
		
	block = this->program.get_block_ptr(this->program.get_block_id("main"));

	if (block->native)
	{
		block->native(&stack, &memory);
		goto cleanup;
	}	
		
    DISPATCH
		OP(LIT_INT)
			push<long>(instr->arg.l);
			NEXT
		OP(LIT_FLOAT)
			push<double>(instr->arg.d);
			NEXT
		OP(LIT_CHAR)
			push<char>(instr->arg.c);
			NEXT
		OP(LOAD_INT)
			push<long>(get_memory<int>(instr->arg.l));
			NEXT
		OP(LOAD_FLOAT)
			push<double>(get_memory<double>(instr->arg.l));
			NEXT
		OP(LOAD_CHAR)
			push<char>(get_memory<char>(instr->arg.l));
			NEXT
		OP(STORE_INT)
			store_memory(instr->arg.l, pop<int>());
			NEXT
		OP(STORE_FLOAT) 
			store_memory(instr->arg.l, pop<double>());
			NEXT
		OP(STORE_CHAR)
			store_memory(instr->arg.c, pop<char>());
			NEXT
		OP(ADD_INT)
			push<long>(pop<int>() + pop<int>());
			NEXT
		OP(ADD_FLOAT)
			push<double>(pop<double>() + pop<double>());
			NEXT
		OP(SUB_INT)
			push<long>(pop<int>() - pop<int>());
			NEXT
		OP(SUB_FLOAT)
			push<double>(pop<double>() - pop<double>());
			NEXT
		OP(MUL_INT)
			push<long>(pop<int>() * pop<int>());
			NEXT
		OP(MUL_FLOAT)
			push<double>(pop<double>() * pop<double>());
			NEXT
		OP(DIV_INT)
			push<long>(pop<int>() / pop<int>());
			NEXT
		OP(DIV_FLOAT)
			push<double>(pop<double>() / pop<double>());
			NEXT
		OP(MOD_INT)
			push<long>(pop<int>() % pop<int>());
			NEXT
		OP(MOD_FLOAT)
			push<double>(fmod(pop<double>(), pop<double>()));
			NEXT
		OP(AND_INT)
			push<long>(pop<int>() & pop<int>());
			NEXT
		OP(OR_INT)
			push<long>(pop<int>() | pop<int>());
			NEXT
		OP(LE_INT)
			push<bool>(pop<int>() < pop<int>());
			NEXT
		OP(LE_FLOAT)
			push<bool>(pop<double>() < pop<double>());
			NEXT
		OP(LEQ_INT)
			push<bool>(pop<int>() <= pop<int>());
			NEXT
		OP(LEQ_FLOAT)
			push<bool>(pop<double>() <= pop<double>());
			NEXT
		OP(GE_INT)
			push<bool>(pop<int>() > pop<int>());
			NEXT
		OP(GE_FLOAT)
			push<bool>(pop<double>() > pop<double>());
			NEXT
		OP(GEQ_INT)
			push<bool>(pop<int>() >= pop<int>());
			NEXT
		OP(GEQ_FLOAT)
			push<bool>(pop<double>() >= pop<double>());
			NEXT
		OP(EQ_INT)
			push<bool>(pop<int>() == pop<int>());
			NEXT
		OP(EQ_FLOAT)
			push<bool>(pop<double>() == pop<double>());
			NEXT
		OP(NEQ_INT)
			push<bool>(pop<int>() != pop<int>());
			NEXT
		OP(NEQ_FLOAT)
			push<bool>(pop<double>() != pop<double>());
			NEXT
		OP(TJMP)
			if (pop<bool>())
				ip = instr->arg.l;
			NEXT
		OP(FJMP)
			if (!pop<bool>())
				ip = instr->arg.l;
			NEXT
		OP(UJMP)
			ip = instr->arg.l;
			NEXT
		OP(PRINT_INT)
			printf("%d", pop<int>());
			NEXT
		OP(PRINT_FLOAT)
			printf("%f", pop<double>());
			NEXT
		OP(PRINT_CHAR)
			putchar(pop<char>());
			NEXT
		OP(NEW_ARY)
			push<Variant*>(manager.new_lang_array(instr->arg.l));
			NEXT
		OP(STORE_ARY)
			store_memory(instr->arg.l, pop<Variant*>());
			NEXT
		OP(LOAD_ARY)
			push<Variant*>(get_memory<Variant*>(instr->arg.l));
			NEXT
		OP(CPY_ARY_CHAR)
			{
				const std::string& string = program.get_string(instr->arg.l);
				Variant* array = pop<Variant*>();

				for (long i = 1; i < array[0].l; ++i)
					array[i].c = string[i-1];

				push<Variant*>(array);
			}
			NEXT
		OP(PRINT_ARY_CHAR)
			{
				Variant* array = pop<Variant*>();
				for (long i = 1; i < array[0].l; ++i)
					putchar(array[i].c);
			}
			NEXT
		OP(CALL)
			memory += block->get_memory_slots();
			push_frame(Frame(ip, block));
			block = reinterpret_cast<Block*>(instr->arg.p);
			ip = 0;		
			NEXT
		OP(CALL_NATIVE)
			{
			Block* callee = reinterpret_cast<Block*>(instr->arg.p);
			memory += block->get_memory_slots();
			callee->native(&stack, &memory);
			memory -= block->get_memory_slots();
			}				
			NEXT
		OP(RTRN)
			if (likely(!frames_is_empty()))
			{
				Frame f = pop_frame();
				block = f.block;
				ip = f.ip;	
				
				memory -= block->get_memory_slots();						
				// memset(memory+1, 0, block->get_memory_slots());

				/*
				Right now, the GC reads this stack for references to other active
				allocated objects, because we just reset our position, if they are
				never overwritten, the GC will never collect. Issue to work on.

				for (int i = sp; i > f.spc; --i)
					memory[i] = 0;*/	
			}
			else
			{
				if (unlikely(debug))
				{
					printf("--------------------\n");
					printf("Instructions executed: %d\n", ipc);
					printf("Yielding stack...\n");
					while (!stack_is_empty())
						printf("%d\n", pop<int>());
				}

				// clean and final exit
				goto cleanup;
			}
			NEXT
		OP(STORE_ARY_ELM_CHAR)
			(get_memory<Variant*>(instr->arg.l))[1 + pop<int>()] = pop<long>();
			NEXT
		OP(STORE_ARY_ELM_INT)
			(get_memory<Variant*>(instr->arg.l))[1 + pop<int>()] = pop<long>();
			NEXT
		OP(STORE_ARY_ELM_FLOAT)
			(get_memory<Variant*>(instr->arg.l))[1 + pop<int>()] = pop<double>();
			NEXT
		OP(LOAD_ARY_ELM_CHAR)
			push<char>((get_memory<Variant*>(instr->arg.l))[1 + pop<long>()].c);
			NEXT
		OP(LOAD_ARY_ELM_INT)
			push<long>((get_memory<Variant*>(instr->arg.l))[1 + pop<long>()].l);
			NEXT
		OP(LOAD_ARY_ELM_FLOAT)
			push<double>((get_memory<Variant*>(instr->arg.l))[1 + pop<long>()].d);
			NEXT
			
		OP(LIT_LOAD_ADD)
			push<long>(get_memory<int>(instr->arg.l) + instr->arg2.l);
			NEXT
		OP(LIT_LOAD_SUB)
			push<long>(get_memory<int>(instr->arg.l) - instr->arg2.l);
			NEXT
		OP(LIT_LOAD_LE)
			push<long>(get_memory<int>(instr->arg.l) < instr->arg2.l);
			NEXT

		OP(F2I)
			push<long>(pop<double>());
			NEXT
		OP(I2F)
			push<double>(pop<int>());
			NEXT
			
		OP(LBL)
			raise_error("fatal label encounter in virtual machine");
			NEXT
	END_DISPATCH
	
	cleanup:
		program.clean_up();
		return;
}
