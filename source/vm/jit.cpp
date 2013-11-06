#include "block.h"
#include "common.h"

#include <algorithm>
#include <stack>

#include "AsmJit/AsmJit.h"
using namespace AsmJit;

void putdouble(long l)
{
	double n;
	memcpy(&n, &l, sizeof(double));
	printf("%f", n);
}

void putint(int n)
{
	printf("%d", n);
}

void Block::jit(std::vector<Block*>& blocks)
{
#ifndef ASMJIT_X64
	return;
#else

	Compiler c;
	FileLogger logger(stderr);
	c.setLogger(&logger);

	// Tell compiler the function prototype we want. It allocates variables representing
	// function arguments that can be accessed through Compiler or Function instance.
	EFunction* func = c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<Void, int64_t*, int64_t*>());
	c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
	c.comment(std::string("jit function '" + this->get_name() + "'").c_str());
	
	Label L_Return = c.newLabel();

	// function arguments
	GPVar stack(c.argGP(0));
	GPVar memory(c.argGP(1));

	GPVar stackTop(c.newGP());
	c.mov(stackTop, qword_ptr(stack));
	GPVar memoryTop(c.newGP());
	c.mov(memoryTop, qword_ptr(memory));

	std::map<int, Label> label_positions;
	for (const auto& instr : instructions)
		if (instr.op == TJMP || instr.op == FJMP || instr.op == UJMP)
			label_positions[instr.arg.l] = c.newLabel();

	int instr_count = 0;
	for (std::vector<Instruction>::iterator instr = instructions.begin(); instr != instructions.end(); ++instr)
	{
		std::map<int, Label>::iterator label_position = label_positions.find(instr_count);
		if (label_position != label_positions.end())
			c.bind(label_position->second);
		
		++instr_count;
		
		switch (instr->op)
		{
			case LIT_INT:
			case LIT_CHAR:
				{
				c.comment("LIT_INT/CHAR");

				long lit = 0;
				switch (instr->op)
				{
					case LIT_INT:
						lit = instr->arg.l;
						break;
					case LIT_CHAR:
						lit = instr->arg.c;
						break;
				}

				c.add(stackTop, 8);
				c.mov(qword_ptr(stackTop), lit);
				}
				break;

			case LIT_FLOAT:
				{
				c.comment("LIT_FLOAT");

				GPVar tmp(c.newGP());
				GPVar address(c.newGP());

				c.mov(address, imm((sysint_t)(void*)&instr->arg.d));
				c.mov(tmp, qword_ptr(address));

				c.add(stackTop, 8);
				c.mov(qword_ptr(stackTop), tmp);
				}
				break;

			case LOAD_INT:
			case LOAD_CHAR:
			case LOAD_FLOAT:
				{
				c.comment("LOAD_INT/CHAR/FLOAT");
				GPVar tmp(c.newGP());
				c.mov(tmp, qword_ptr(memoryTop, (instr->arg.l * 8)));

				c.add(stackTop, 8);
				c.mov(qword_ptr(stackTop), tmp);

				c.unuse(tmp);
				}
				break;

			case STORE_INT:
			case STORE_CHAR:
			case STORE_FLOAT:
				{
				c.comment("STORE_INT/CHAR/FLOAT");
				GPVar tmp(c.newGP());
				c.mov(tmp, qword_ptr(stackTop));
				c.sub(stackTop, 8);

				c.mov(qword_ptr(memoryTop, (instr->arg.l * 8)), tmp);

				c.unuse(tmp);
				}
				break;
				
			case ADD_INT:
				{
				c.comment("ADD_INT");
				
				GPVar tmp(c.newGP());
				c.mov(tmp, qword_ptr(stackTop));
				
				c.add(qword_ptr(stackTop, -8), tmp);
				c.sub(stackTop, 8);
				
				c.unuse(tmp);
				}
				break;
			
			case MUL_INT:
			case SUB_INT:
				{
				c.comment("ARITH_INT");
				GPVar tmp1(c.newGP());
				GPVar tmp2(c.newGP());

				c.mov(tmp1, qword_ptr(stackTop));
				c.mov(tmp2, qword_ptr(stackTop, -8));

				switch (instr->op)
				{
					case MUL_INT:
						c.imul(tmp1, tmp2);
						break;
					case SUB_INT:
						c.sub(tmp1, tmp2);
						break;
					case ADD_INT:
						c.add(tmp1, tmp2);
						break;
				}

				c.sub(stackTop, 8);
				c.mov(qword_ptr(stackTop), tmp1);

				c.unuse(tmp1);
				c.unuse(tmp2);
				}
				break;

			case DIV_INT:
			case MOD_INT:
				{
				c.comment("MOD_INT");
				GPVar tmp0(c.newGP());
				GPVar tmp1(c.newGP());
				// GPVar tmp2(c.newGP());

				c.mov(tmp1, qword_ptr(stackTop));

				// make rdx 0, but really should sign extend it with cqo
				GPVar t_rdx(c.newGP());
				c.alloc(t_rdx, rdx);
				c.xor_(t_rdx, t_rdx);
				c.unuse(t_rdx);

				c.idiv(tmp0, tmp1, qword_ptr(stackTop, -8));

				c.sub(stackTop, 8);

				switch (instr->op)
				{
					case DIV_INT:
						c.mov(qword_ptr(stackTop), tmp1);
						break;
					case MOD_INT:
						c.mov(qword_ptr(stackTop), tmp0);
						break;
				}

				c.unuse(tmp0);
				c.unuse(tmp1);
				}
				break;

			case ADD_FLOAT:
			case SUB_FLOAT:
			case MUL_FLOAT:
			case DIV_FLOAT:
				{
				c.comment("ARITH_FLOAT");

				XMMVar tmp1(c.newXMM());
				XMMVar tmp2(c.newXMM());

				c.movsd(tmp1, qword_ptr(stackTop));
				c.movsd(tmp2, qword_ptr(stackTop, -8));

				switch (instr->op)
				{
					case ADD_FLOAT:
						c.addsd(tmp1, tmp2);
						break;
					case SUB_FLOAT:
						c.subsd(tmp1, tmp2);
						break;
					case MUL_FLOAT:
						c.mulsd(tmp1, tmp2);
						break;
					case DIV_FLOAT:
						c.divsd(tmp1, tmp2);
						break;
				}

				c.sub(stackTop, 8);
				c.movsd(qword_ptr(stackTop), tmp1);
				}
				break;

			case LEQ_INT:
			case LE_INT:
			case GE_INT:
			case GEQ_INT:
			case EQ_INT:
			case NEQ_INT:
				{
				c.comment("LEQ/LE/GE/GEQ/EQ/NEQ_INT");
				GPVar tmp1(c.newGP());
				GPVar tmp2(c.newGP());

				c.mov(tmp1, qword_ptr(stackTop));
				c.mov(tmp2, qword_ptr(stackTop, -8));

				c.sub(stackTop, 8);

				// branch
				Label L_LT = c.newLabel();
			  	Label L_Exit = c.newLabel();

				c.cmp(tmp1, tmp2);

				switch (instr->op)
				{
					case LEQ_INT:
						c.jle(L_LT);
						break;
					case LE_INT:
						c.jl(L_LT);
						break;
					case GE_INT:
						c.jg(L_LT);
						break;
					case GEQ_INT:
						c.jge(L_LT);
						break;
					case EQ_INT:
						c.je(L_LT);
						break;
					case NEQ_INT:
						c.jne(L_LT);
						break;
				}

				c.mov(qword_ptr(stackTop), imm(0));
				c.jmp(L_Exit);

				c.bind(L_LT);
				c.mov(qword_ptr(stackTop), imm(1));

				c.bind(L_Exit);

				c.unuse(tmp1);
				c.unuse(tmp2);
				}
				break;

				case LEQ_FLOAT:
				case LE_FLOAT:
				case GE_FLOAT:
				case GEQ_FLOAT:
				case EQ_FLOAT:
				case NEQ_FLOAT:
					{
					c.comment("LEQ/LE/GE/GEQ/EQ/NEQ_FLOAT");
					XMMVar tmp1(c.newXMM());
					XMMVar tmp2(c.newXMM());

					c.movsd(tmp1, qword_ptr(stackTop));
					c.movsd(tmp2, qword_ptr(stackTop, -8));

					c.sub(stackTop, 8);

					// branch
					Label L_LT = c.newLabel();
				  	Label L_Exit = c.newLabel();

					c.comisd(tmp1, tmp2);

					switch (instr->op)
					{
						case LEQ_FLOAT:
							c.jb(L_LT);
							break;
						case LE_FLOAT:
							c.jbe(L_LT);
							break;
						case GE_FLOAT:
							c.ja(L_LT);
							break;
						case GEQ_FLOAT:
							c.jae(L_LT);
							break;
						case EQ_FLOAT:
							c.je(L_LT);
							break;
						case NEQ_FLOAT:
							c.jne(L_LT);
							break;
					}

					c.mov(qword_ptr(stackTop), imm(0));
					c.jmp(L_Exit);

					c.bind(L_LT);
					c.mov(qword_ptr(stackTop), imm(1));

					c.bind(L_Exit);

					c.unuse(tmp1);
					c.unuse(tmp2);
					}
					break;

			case TJMP:
			case FJMP:
				{
				c.comment("FJMP");
				GPVar tmp(c.newGP());

				c.mov(tmp, qword_ptr(stackTop));
				c.sub(stackTop, 8);

				switch (instr->op)
				{
					case TJMP:
						c.cmp(tmp, 1);
						break;
					case FJMP:
						c.cmp(tmp, 0);
						break;
				}

				c.jz(label_positions[instr->arg.l]);
				c.unuse(tmp);
				}
				break;

			case UJMP:
				c.comment("UJMP");
				c.jmp(label_positions[instr->arg.l]);
				break;

			case CALL:
				raise_error("'" + reinterpret_cast<Block*>(instr->arg.p)->get_name() + "' must be jit'able to be called from a jit'd function");
				break;
			case CALL_NATIVE:
				{
					std::vector<Block*>::iterator callee = std::find(blocks.begin(), blocks.end(), (Block*)instr->arg.p);
					if (callee == blocks.end())
						raise_error("couldn't identify block for jit native call");

					if ((*callee)->get_jit_type() != BASIC)
						raise_error("all referenced functions must be jittabled");

					if (!(*callee)->native && *callee != this)
						(*callee)->jit(blocks);

					c.comment(std::string("CALL_NATIVE '" + (*callee)->get_name() + "'").c_str());

					c.mov(qword_ptr(stack), stackTop);
					if (this->get_memory_slots() > 0)
						c.add(qword_ptr(memory), 8 * this->get_memory_slots());

					ECall* ctx = NULL;
					if (*callee == this)
						ctx = c.call(func->getEntryLabel());
					else
						ctx = c.call(imm((sysint_t)(void*)(*callee)->native));

					GPVar ret(c.newGP());

					ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, long*, long*>());
					ctx->setArgument(0, stack);
					ctx->setArgument(1, memory);
					ctx->setReturn(ret);

					c.mov(stackTop, qword_ptr(stack));

					if ((*callee)->get_jit_type() == OPTIMIZING)
					{
						c.add(stackTop, 8);
						c.mov(qword_ptr(stackTop), ret);
						c.unuse(ret);
					}

					if (this->get_memory_slots() > 0)
						c.sub(qword_ptr(memory), 8 * this->get_memory_slots());
				}
				break;

			case I2F:
				{
				c.comment("I2F");
				XMMVar tmp1(c.newXMM());
				c.cvtsi2sd(tmp1, qword_ptr(stackTop));
				c.movsd(qword_ptr(stackTop), tmp1);
				c.unuse(tmp1);
				}
				break;

			case F2I:
				{
				c.comment("F2I");
				GPVar tmp(c.newGP());
				c.cvtsd2si(tmp, qword_ptr(stackTop));
				c.mov(qword_ptr(stackTop), tmp);
				c.unuse(tmp);
				}
				break;

			case RTRN:
				// would be better to have ret here, but weird bugs trying to finish jit'd function
				// is it preferable to have a single exit?
				c.comment("RTRN");
				if ((instr+1) != instructions.end() || ((instr+1)->op != RTRN && (instr+1+1) == instructions.end()))
					c.jmp(L_Return);
				break;

			case PRINT_INT:
			case PRINT_CHAR:
			case PRINT_FLOAT:
				{
					c.comment("PRINT_INT/CHAR/FLOAT");
					GPVar tmp(c.newGP());
					c.mov(tmp, qword_ptr(stackTop));
					c.sub(stackTop, 8);

					void* printFunction = NULL;
					if (instr->op == PRINT_INT)
						printFunction = reinterpret_cast<void*>(&putint);
					else if (instr->op == PRINT_FLOAT)
						printFunction = reinterpret_cast<void*>(&putdouble);
					else
						printFunction = reinterpret_cast<void*>(&putchar);

					ECall* ctx = c.call(imm((sysint_t)printFunction));
					ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<Void, void*>());
					ctx->setArgument(0, tmp);

					c.unuse(tmp);
				}
				break;

			case LIT_LOAD_ADD:
			case LIT_LOAD_SUB:
				{
				c.comment("LIT_LOAD_ADD/SUB");
				GPVar tmp(c.newGP());
				c.mov(tmp, qword_ptr(memoryTop, (instr->arg.l * 8)));

				switch (instr->op)
				{
					case LIT_LOAD_ADD:
						c.add(tmp, instr->arg2.l);
						break;
					case LIT_LOAD_SUB:
						c.sub(tmp, instr->arg2.l);
						break;
				}

				c.add(stackTop, 8);
				c.mov(qword_ptr(stackTop), tmp);

				c.unuse(tmp);
				}
				break;

			case LIT_LOAD_LE:
				{
				c.comment("LIT_LOAD_LE");

				GPVar tmp(c.newGP());
				c.mov(tmp, qword_ptr(memoryTop, (instr->arg.l * 8)));

				Label L_LE = c.newLabel();
			  	Label L_Exit = c.newLabel();

				c.cmp(tmp, instr->arg2.l);
				c.jl(L_LE);

				c.mov(tmp, 0);
				c.jmp(L_Exit);

				c.bind(L_LE);
				c.mov(tmp, 1);

				c.bind(L_Exit);

				c.add(stackTop, 8);
				c.mov(qword_ptr(stackTop), tmp);

				c.unuse(tmp);
				}
				break;

			default:
				raise_error("unknown op in jit loop for block '" + this->get_name() + "'");
				break;
		}
	}

	c.bind(L_Return);

	c.mov(qword_ptr(stack), stackTop);

	c.ret();
	c.endFunction();

	this->native = function_cast<native_ptr>(c.make());
	if (!this->native)
		raise_error("unable to create jit'd block");
#endif // ASMJIT_X64
}

void Block::optimizing_jit(std::vector<Block*>& blocks)
{
#ifndef ASMJIT_X64
	return;
#else

	Compiler c;
	FileLogger logger(stderr);
	c.setLogger(&logger);

	// Tell compiler the function prototype we want. It allocates variables representing
	// function arguments that can be accessed through Compiler or Function instance.
	EFunction* func = c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, int64_t*, int64_t*>());
	c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
	c.comment(std::string("optimized jit function '" + this->get_name() + "'").c_str());

	std::map<int, Label> label_positions;
	for (const auto& instr : instructions)
		if (instr.op == TJMP || instr.op == FJMP || instr.op == UJMP)
			label_positions[instr.arg.l] = c.newLabel();
		
	std::stack<BaseVar> stack;
	std::map<int, BaseVar> memory;

	int instr_count = 0;
	for (std::vector<Instruction>::iterator instr = instructions.begin(); instr != instructions.end(); ++instr)
	{
		std::map<int, Label>::iterator label_position = label_positions.find(instr_count);
		if (label_position != label_positions.end())
			c.bind(label_position->second);
		
		++instr_count;
		
		switch (instr->op)
		{
			case LIT_INT:
				{
					c.comment("LIT_INT");
					GPVar tmp(c.newGP());
					c.mov(tmp, instr->arg.l);

					stack.push(tmp);
				}
				break;

			case LOAD_INT:
			case LOAD_CHAR:
			case LOAD_FLOAT:
				{
				c.comment("LOAD_INT/CHAR/FLOAT");
				stack.push(memory[instr->arg.l]);
				}
				break;

			case STORE_INT:
			case STORE_CHAR:
			case STORE_FLOAT:
				{
				c.comment("STORE_INT/CHAR/FLOAT");
				
				GPVar tmp = static_cast<GPVar&>(stack.top()); stack.pop();

				if (memory.find(instr->arg.l) != memory.end())
					c.unuse(memory[instr->arg.l]);
				
				memory[instr->arg.l] = tmp;
				}
				break;

			case LEQ_INT:
			case LE_INT:
			case GE_INT:
			case GEQ_INT:
			case EQ_INT:
			case NEQ_INT:		
				{
					c.comment("CMP_INT");
					GPVar tmp1 = static_cast<GPVar&>(stack.top()); stack.pop();
					GPVar tmp2 = static_cast<GPVar&>(stack.top()); stack.pop();

					// branch
					Label L_LT = c.newLabel();
				  	Label L_Exit = c.newLabel();

					GPVar cond(c.newGP());
					c.cmp(tmp2, tmp1);

					switch (instr->op)
					{
						case LEQ_INT:
							c.jle(L_LT);
							break;
						case LE_INT:
							c.jl(L_LT);
							break;
						case GE_INT:
							c.jg(L_LT);
							break;
						case GEQ_INT:
							c.jge(L_LT);
							break;
						case EQ_INT:
							c.je(L_LT);
							break;
						case NEQ_INT:
							c.jne(L_LT);
							break;
					}

					c.mov(cond, 0);
					c.jmp(L_Exit);

					c.bind(L_LT);
					c.mov(cond, 1);
					c.bind(L_Exit);

					c.unuse(tmp1);
					c.unuse(tmp2);

					stack.push(cond);
				}
				break;

			case TJMP:
			case FJMP:
				{
				c.comment("FJMP/TJMP");
				GPVar tmp = static_cast<GPVar&>(stack.top()); stack.pop();

				switch (instr->op)
				{
					case TJMP:
						c.cmp(tmp, 1);
						break;
					case FJMP:
						c.cmp(tmp, 0);
						break;
				}

				c.jz(label_positions[instr->arg.l]);
				c.unuse(tmp);
				}
				break;

			case ADD_INT:
			case SUB_INT:
			case MUL_INT:
				{
					c.comment("MATH_INT");
					GPVar a = static_cast<GPVar&>(stack.top()); stack.pop();
					GPVar b = static_cast<GPVar&>(stack.top()); stack.pop();

					switch (instr->op) 
					{
						case ADD_INT:
							c.add(a, b);
							break;
						case SUB_INT:
							c.sub(a, b);
							break;
						case MUL_INT:
							c.imul(a, b);
							break;
						default:
							raise_error("failure");
							break;
					}

					stack.push(a);
					c.unuse(b);
				}
				break;

			case UJMP:
				c.comment("UJMP");
				c.jmp(label_positions[instr->arg.l]);
				break;

			case CALL:
				raise_error("'" + reinterpret_cast<Block*>(instr->arg.p)->get_name() + "' must be jit'able to be called from a jit'd function");
				break;
			case CALL_NATIVE:
				{
					std::vector<Block*>::iterator callee = std::find(blocks.begin(), blocks.end(), (Block*)instr->arg.p);
					if (callee == blocks.end())
						raise_error("couldn't identify block for jit native call");

					if ((*callee)->get_jit_type() != OPTIMIZING)
						raise_error("all referenced functions must be jittabled and optimizing");

					if (!(*callee)->native && *callee != this)
						(*callee)->jit(blocks);

					c.comment(std::string("CALL_NATIVE '" + (*callee)->get_name() + "'").c_str());

					GPVar s(c.argGP(0));
					GPVar m(c.argGP(1));
		
					if (this->get_memory_slots() > 0)
						c.add(qword_ptr(m), 8 * this->get_memory_slots());

					ECall* ctx = NULL;
					if (*callee == this)
						ctx = c.call(func->getEntryLabel());
					else
						ctx = c.call(imm((sysint_t)(void*)(*callee)->native));

					GPVar ret(c.newGP());

					ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, long*, long*>());
					ctx->setArgument(0, s);
					ctx->setArgument(1, m);
					ctx->setReturn(ret);

					if ((*callee)->get_jit_type() == OPTIMIZING)
						stack.push(static_cast<BaseVar&>(ret));

					if (this->get_memory_slots() > 0)
						c.sub(qword_ptr(m), 8 * this->get_memory_slots());
				}
				break;


			case RTRN:
				// TODO fix floats
				{
					if (stack.size() >= 1)
					{
						BaseVar tmp = stack.top(); stack.pop();
						c.ret(static_cast<GPVar&>(tmp));
					}
					else
					{
						c.ret();
					}
				}
				break;

			case LIT_LOAD_LE:
				{
				c.comment("LIT_LOAD_LE");

				GPVar tmp = static_cast<GPVar&>(memory[instr->arg.l]);
				GPVar ret(c.newGP());

				Label L_LE = c.newLabel();
			  	Label L_Exit = c.newLabel();

				c.cmp(tmp, instr->arg2.l);
				c.jl(L_LE);

				c.mov(ret, 0);
				c.jmp(L_Exit);

				c.bind(L_LE);
				c.mov(ret, 1);

				c.bind(L_Exit);

				stack.push(static_cast<BaseVar&>(ret));
				}
				break;	


			case LIT_LOAD_ADD:
				{
				c.comment("LIT_LOAD_ADD");
				GPVar tmp = static_cast<GPVar&>(memory[instr->arg.l]);
				c.add(tmp, instr->arg2.l);
				stack.push(static_cast<BaseVar&>(tmp));
				}
				break;		

			default:
				raise_error("unimplemented opcode " + opcode_str[instr->op] + " in optimizing jit");
				break;
		}
	}

	c.endFunction();

	this->native = function_cast<native_ptr>(c.make());
	if (!this->native)
		raise_error("unable to create jit'd block");
#endif // ASMJIT_X64
}
