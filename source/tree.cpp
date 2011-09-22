#include "tree.h"

BuildContext::BuildContext(bool nojit) : 
	program(Program()),
	working_block(NULL),
	memory_ids(std::map<std::string, int>()),
	block_types(std::map<std::string, Type>()),
	variable_types(std::map<std::string, Type>()),
	seed(0),
	nojit(nojit)
{
}

Program& BuildContext::get_program()
{
	return program;
}

Block* BuildContext::get_block()
{
	return working_block;
}

void BuildContext::set_block(Block* block)
{	
	// clear out memory and variables
	memory_ids.clear();
	variable_types.clear();

	// now reset the block
	working_block = block;
	
	// reset seed
	seed = 0;
}

int BuildContext::get_mem_id(const std::string& key)
{
	std::map<std::string, int>::iterator it = memory_ids.find(key);
	if (it == memory_ids.end())
	{
		// the key wasn't found
		memory_ids[key] = memory_ids.size();
		return memory_ids[key];
	}
	else
	{
		return (*it).second;
	}
}

void BuildContext::set_variable_type(const std::string& key, Type value)
{
	variable_types[key] = value;
}

Type BuildContext::get_variable_type(const std::string& key)
{
	return variable_types[key];
}

void BuildContext::set_block_type(const std::string& key, Type value)
{
	block_types[key] = value;
}

Type BuildContext::get_block_type(const std::string& key)
{
	return block_types[key];
}

bool BuildContext::variable_exists(const std::string& key)
{
	return (memory_ids.find(key) != memory_ids.end());
}

int BuildContext::get_and_increment_seed()
{
	return this->seed++;
}

bool BuildContext::get_nojit() const
{
	return this->nojit;
}

std::string CallExprAST::callee_signature(BuildContext& out)
{
	std::string block_name = this->callee;

	for (std::vector<ExprAST*>::iterator i = Args.begin(); i != Args.end(); ++i)
	{
		if (CallExprAST* call_ast = dynamic_cast<CallExprAST*>(*i))
			(*i)->type = out.get_block_type(call_ast->callee_signature(out));
		else if (VariableExprAST* var_ast = dynamic_cast<VariableExprAST*>(*i))
			(*i)->type = out.get_variable_type(var_ast->name);

		block_name += (std::string(":") + (*i)->type.get_name());
	}

	return block_name;
}

int CallExprAST::get_block_id(BuildContext& out, Block* block)
{
 	return out.get_program().get_block_id(callee_signature(out));
}

void ProgramAST::register_functions(BuildContext& out)
{
	for (std::vector<FunctionAST*>::iterator i = functions.begin(); i != functions.end(); ++i)
	{
		out.get_program().add_block(new Block((*i)->proto->name));
		out.set_block_type((*i)->proto->name, (*i)->proto->type);		
	}
}

void ProgramAST::emit_bytecode(BuildContext& out)
{
	for (std::vector<FunctionAST*>::iterator i = functions.begin(); i != functions.end(); ++i)
		(*i)->emit_bytecode(out);
}

ProgramAST::~ProgramAST()
{
	for (std::vector<FunctionAST*>::iterator i = functions.begin(); i != functions.end(); ++i)
		delete *i;
}
