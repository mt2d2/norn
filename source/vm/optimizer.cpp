/* 
Constant folding in source/vm/optimizer.cpp was heavily influenced by Objeck 
(http://objeck-lang.sourceforge.net/), whose LICENSE is as follows. 
 
 **************************************************************************
 * Copyright (c) 2008-2011, Randy Hollines
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * - Neither the name of the Objeck Team nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#include "block.h"

#include <cmath> // fmod

void Block::store_load_elimination()
{
	int line_number = 0;
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
	{
		++line_number;

		if (i->op == STORE_INT)
			if ((i+1)->op == LOAD_INT && (i->arg.l == (i+1)->arg.l))
				instructions.erase(i, i+1+1); // exclusive, i.e., upto

		if (i->op == STORE_FLOAT)
			if ((i+1)->op == LOAD_FLOAT && (i->arg.l == (i+1)->arg.l))
				instructions.erase(i, i+1+1); // exclusive, i.e., upto
		
		if (i->op == STORE_CHAR)
			if ((i+1)->op == LOAD_CHAR && (i->arg.l == (i+1)->arg.l))
				instructions.erase(i, i+1+1); // exclusive, i.e., upto
		
		if (i->op == STORE_ARY)
			if ((i+1)->op == LOAD_ARY && (i->arg.l == (i+1)->arg.l))
				instructions.erase(i, i+1+1); // exclusive, i.e., upto
	}
}

void Block::calculate_int_fold(Instruction instr, std::list<Instruction>& calc_stack, std::list<Instruction>& outputs)
{
	if(calc_stack.size() == 1) 
	{
		outputs.push_back(calc_stack.front());
		calc_stack.pop_front();
		outputs.push_back(instr);
	} 
	else if(calc_stack.size() > 1) 
	{
		Instruction& left = calc_stack.front();
		calc_stack.pop_front();

		Instruction& right = calc_stack.front();
		calc_stack.pop_front();

		switch (instr.op) 
		{
			case ADD_INT:
				calc_stack.push_front(Instruction(LIT_INT, left.arg.l + right.arg.l));
				break;

			case SUB_INT:
				calc_stack.push_front(Instruction(LIT_INT, left.arg.l - right.arg.l));
				break;

			case MUL_INT:
				calc_stack.push_front(Instruction(LIT_INT, left.arg.l * right.arg.l));
				break;

			case DIV_INT:
				calc_stack.push_front(Instruction(LIT_INT, left.arg.l / right.arg.l));
				break;

			case MOD_INT:
				calc_stack.push_front(Instruction(LIT_INT, left.arg.l + right.arg.l));
				break;
		}
	} 
	else 
	{
		outputs.push_back(instr);
	}
}

void Block::calculate_float_fold(Instruction instr, std::list<Instruction>& calc_stack, std::list<Instruction>& outputs)
{
	if(calc_stack.size() == 1) 
	{
		outputs.push_back(calc_stack.front());
		calc_stack.pop_front();
		outputs.push_back(instr);
	} 
	else if(calc_stack.size() > 1) 
	{
		Instruction& left = calc_stack.front();
		calc_stack.pop_front();

		Instruction& right = calc_stack.front();
		calc_stack.pop_front();

		switch (instr.op) 
		{
			case ADD_FLOAT:
				calc_stack.push_front(Instruction(LIT_FLOAT, left.arg.d + right.arg.d));
				break;

			case SUB_FLOAT:
				calc_stack.push_front(Instruction(LIT_FLOAT, left.arg.d - right.arg.d));
				break;

			case MUL_FLOAT:
				calc_stack.push_front(Instruction(LIT_FLOAT, left.arg.d * right.arg.d));
				break;

			case DIV_FLOAT:
				calc_stack.push_front(Instruction(LIT_FLOAT, left.arg.d / right.arg.d));
				break;

			case MOD_FLOAT:
				calc_stack.push_front(Instruction(LIT_FLOAT, fmod(left.arg.d, right.arg.d)));
				break;
		}
	} 
	else 
	{
		outputs.push_back(instr);
	}
}

void Block::fold_ints()
{
	std::list<Instruction> outputs;
	std::list<Instruction> calc_stack;
	
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
	{
		switch (i->op)
		{
			case LIT_INT:
				calc_stack.push_front(*i);
				break;

			case ADD_INT:
			case SUB_INT:
			case MUL_INT:
			case DIV_INT:
			case MOD_INT:
				calculate_int_fold(*i, calc_stack, outputs);
				break;

			default:
				// add back in reverse order
				while(!calc_stack.empty()) 
				{
					outputs.push_back(calc_stack.back());
					calc_stack.pop_back();
				}
				
				outputs.push_back(*i);
				break;
		}
	}

	// order matters...
	while (!calc_stack.empty())
	{
		outputs.push_back(calc_stack.back());
		calc_stack.pop_back();
	}
	
	instructions.clear();
	for (std::list<Instruction>::iterator i = outputs.begin(); i != outputs.end(); ++i)
		instructions.push_back(*i);
}

void Block::fold_floats()
{
	std::list<Instruction> outputs;
	std::list<Instruction> calc_stack;
	
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
	{
		switch (i->op)
		{
			case LIT_FLOAT:
				calc_stack.push_front(*i);
				break;

			case ADD_FLOAT:
			case SUB_FLOAT:
			case MUL_FLOAT:
			case DIV_FLOAT:
			case MOD_FLOAT:
				calculate_float_fold(*i, calc_stack, outputs);
				break;

			default:
				// add back in reverse order
				while(!calc_stack.empty()) 
				{
					outputs.push_back(calc_stack.back());
					calc_stack.pop_back();
				}
				
				outputs.push_back(*i);
				break;
		}
	}

	// order matters...
	while (!calc_stack.empty())
	{
		outputs.push_back(calc_stack.back());
		calc_stack.pop_back();
	}
	
	instructions.clear();
	for (std::list<Instruction>::iterator i = outputs.begin(); i != outputs.end(); ++i)
		instructions.push_back(*i);
}

void Block::fold_constants()
{
	fold_ints();
	fold_floats();
}

void Block::inline_calls()
{
	int line_number = 0;
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
	{
		++line_number;
		if (i->op == CALL || i->op == CALL_NATIVE)
		{
			Block* callee = reinterpret_cast<Block*>(i->arg.p);
			
			if (callee->get_size() <= 4)
			{
				instructions.erase(i, i+1); // exclusive, i.e., upto				
				instructions.insert(i, callee->get_instructions().begin(), callee->get_instructions().end()-1);		
			}
		}
	}
	
}

void Block::lit_load_add()
{
	int line_number = 0;
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
	{
		++line_number;

		if (i->op == LIT_INT)
		{
			if ((i+1)->op == LOAD_INT)
			{
				if ((i+1+1)->op == ADD_INT)
				{
					Instruction newInstr(LIT_LOAD_ADD);
					newInstr.arg = (i+1)->arg.l;
					newInstr.arg2 = i->arg.l;

					instructions.erase(i, i+1+1+1); // exclusive, i.e., upto
					instructions.insert(i, newInstr);
				}
			}
		}
	}
}

void Block::lit_load_sub()
{
	int line_number = 0;
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
	{
		++line_number;

		if (i->op == LIT_INT)
		{
			if ((i+1)->op == LOAD_INT)
			{
				if ((i+1+1)->op == SUB_INT)
				{
					Instruction newInstr(LIT_LOAD_SUB);
					newInstr.arg = (i+1)->arg.l;
					newInstr.arg2 = i->arg.l;

					instructions.erase(i, i+1+1+1); // exclusive, i.e., upto
					instructions.insert(i, newInstr);
				}
			}
		}
	}
}

void Block::lit_load_le()
{
	int line_number = 0;
	for (std::vector<Instruction>::iterator i = instructions.begin(); i != instructions.end(); ++i)
	{
		++line_number;

		if (i->op == LIT_INT)
		{
			if ((i+1)->op == LOAD_INT)
			{
				if ((i+1+1)->op == LE_INT)
				{
					Instruction newInstr(LIT_LOAD_LE);
					newInstr.arg = (i+1)->arg.l;
					newInstr.arg2 = i->arg.l;

					instructions.erase(i, i+1+1+1); // exclusive, i.e., upto
					instructions.insert(i, newInstr);
				}
			}
		}
	}
}
