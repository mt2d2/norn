#include "machine.h"
#include <cmath> // fmod
#include <cstdio> // printf

#define BACKEDGE_HOTNESS 40
#define CALL_HOTNESS 40

#define COMPUTED_GOTO __GNUC__
#if COMPUTED_GOTO
#	define DISPATCH NEXT
#	define OP(x) x:
#	define NEXT instr = block->get_instruction(ip++); goto *disp_table[instr->op];
#	define END_DISPATCH
#else
#	define DISPATCH while (true) { instr = block->get_instruction(ip++); /*std::cout << *instr << std::endl;*/ switch(instr->op) {
#	define OP(x) case x:
#	define NEXT break;
#	define END_DISPATCH } }
#endif

// TODO test perf of switch root_arrays to vectors, would make a lot simpler!
// TODO refactor Opcode so reapir_disp_table isn't needed
// TODO reference things by frame, push inital frame for main, too
Machine::Machine(const Program& program, bool debug, bool nojit) :
	program(program),
	block(nullptr),
	instr(nullptr),
	manager(Memory()),
	ip(0),
#if !NOJIT
	debug(debug),
	nojit(nojit),
#endif
	stack(new int64_t[STACK_SIZE]),
	stack_start(stack),	
	frames(new Frame[STACK_SIZE]),
	frames_start(frames),
	memory(new int64_t[STACK_SIZE * this->program.get_memory_slots()])
{
}

Machine::~Machine()
{
	delete[] stack_start;
	delete[] frames_start;
	delete[] memory;
}

void Machine::execute()
{	
#if COMPUTED_GOTO
#	include "goto.h"
#endif

	block = this->program.get_block_ptr(this->program.get_block_id("main"));
		
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
			push<long>(get_memory<long>(instr->arg.l));
			NEXT
		OP(LOAD_FLOAT)
			push<double>(get_memory<double>(instr->arg.l));
			NEXT
		OP(LOAD_CHAR)
			push<char>(get_memory<char>(instr->arg.l));
			NEXT
		OP(STORE_INT)
			store_memory(instr->arg.l, pop<long>());
			NEXT
		OP(STORE_FLOAT) 
			store_memory(instr->arg.l, pop<double>());
			NEXT
		OP(STORE_CHAR)
			store_memory(instr->arg.c, pop<char>());
			NEXT
		OP(ADD_INT)
			push<long>(pop<long>() + pop<long>());
			NEXT
		OP(ADD_FLOAT)
			push<double>(pop<double>() + pop<double>());
			NEXT
		OP(SUB_INT)
			push<long>(pop<long>() - pop<long>());
			NEXT
		OP(SUB_FLOAT)
			push<double>(pop<double>() - pop<double>());
			NEXT
		OP(MUL_INT)
			push<long>(pop<long>() * pop<long>());
			NEXT
		OP(MUL_FLOAT)
			push<double>(pop<double>() * pop<double>());
			NEXT
		OP(DIV_INT)
			push<long>(pop<long>() / pop<long>());
			NEXT
		OP(DIV_FLOAT)
			push<double>(pop<double>() / pop<double>());
			NEXT
		OP(MOD_INT)
			push<long>(pop<long>() % pop<long>());
			NEXT
		OP(MOD_FLOAT)
			push<double>(fmod(pop<double>(), pop<double>()));
			NEXT
		OP(AND_INT)
			push<long>(pop<long>() & pop<long>());
			NEXT
		OP(OR_INT)
			push<long>(pop<long>() | pop<long>());
			NEXT
		OP(LE_INT)
			push<bool>(pop<long>() < pop<long>());
			NEXT
		OP(LE_FLOAT)
			push<bool>(pop<double>() < pop<double>());
			NEXT
		OP(LEQ_INT)
			push<bool>(pop<long>() <= pop<long>());
			NEXT
		OP(LEQ_FLOAT)
			push<bool>(pop<double>() <= pop<double>());
			NEXT
		OP(GE_INT)
			push<bool>(pop<long>() > pop<long>());
			NEXT
		OP(GE_FLOAT)
			push<bool>(pop<double>() > pop<double>());
			NEXT
		OP(GEQ_INT)
			push<bool>(pop<long>() >= pop<long>());
			NEXT
		OP(GEQ_FLOAT)
			push<bool>(pop<double>() >= pop<double>());
			NEXT
		OP(EQ_INT)
			push<bool>(pop<long>() == pop<long>());
			NEXT
		OP(EQ_FLOAT)
			push<bool>(pop<double>() == pop<double>());
			NEXT
		OP(NEQ_INT)
			push<bool>(pop<long>() != pop<long>());
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
			{
#if !NOJIT
				if (likely(!this->nojit))
				{
					if (instr->arg.l < ip)
					{
						block->add_backedge_hotness(instr);

						if (block->get_backedge_hotness(instr) == BACKEDGE_HOTNESS) 
						{
							if (unlikely(this->debug))
								fprintf(stderr, "hot backedge at %s:%d\n", block->get_name().c_str(), ip);

							// compile a special version of the block
							// that bounces to the correct spot in the compiled code
							block->jit(this->program, instr->arg.l);
							block->native(&stack, &memory);
							block->free_native_code();

							// now throw away the previous compiled code
							// recompile the whole block and don't bounce in
							block->jit(this->program);
							goto RETURN;
						}
					}
				}
#endif

				ip = instr->arg.l;
			}
			NEXT
		OP(PRINT_INT)
			printf("%ld", pop<long>());
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

#if !NOJIT
			if (unlikely(block->get_hotness()) == CALL_HOTNESS)
			{
				if (unlikely(this->debug))
					fprintf(stderr, "hot call %s\n", block->get_name().c_str());

				if (likely(!this->nojit))
					block->jit(this->program);
			}

			block->add_hotness();
#endif

			NEXT
		OP(CALL_NATIVE)
			{
			Block* callee = reinterpret_cast<Block*>(instr->arg.p);
			memory += block->get_memory_slots();

#if !NOJIT
			long ret = callee->native(&stack, &memory);
			if (callee->get_jit_type() == OPTIMIZING)
				push<long>(ret);
#else
			callee->native(&stack, &memory);
#endif

			memory -= block->get_memory_slots();
			}				
			NEXT

#if !NOJIT
RETURN:
#endif
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
				// clean and final exit
				goto cleanup;
			}
			NEXT
		OP(STORE_ARY_ELM_CHAR)
			(get_memory<Variant*>(instr->arg.l))[1 + pop<long>()] = pop<long>();
			NEXT
		OP(STORE_ARY_ELM_INT)
			(get_memory<Variant*>(instr->arg.l))[1 + pop<long>()] = pop<long>();
			NEXT
		OP(STORE_ARY_ELM_FLOAT)
			(get_memory<Variant*>(instr->arg.l))[1 + pop<long>()] = pop<double>();
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
			push<long>(get_memory<long>(instr->arg.l) + instr->arg2.l);
			NEXT
		OP(LIT_LOAD_SUB)
			push<long>(get_memory<long>(instr->arg.l) - instr->arg2.l);
			NEXT
		OP(LIT_LOAD_LE)
			push<long>(get_memory<long>(instr->arg.l) < instr->arg2.l);
			NEXT

		OP(F2I)
			push<long>(pop<double>());
			NEXT
		OP(I2F)
			push<double>(pop<long>());
			NEXT
			
		OP(LBL)
			raise_error("fatal label encounter in virtual machine");
			NEXT

		OP(LOGICAL_AND)
			push<bool>(pop<bool>() && pop<bool>());
			NEXT
		OP(LOGICAL_OR)
			push<bool>(pop<bool>() || pop<bool>());
			NEXT
	END_DISPATCH
	
	cleanup:
		program.clean_up();
		return;
}
