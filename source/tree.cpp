#include "tree.h"

BuildContext::BuildContext() : 
	program(Program()),
	working_block(nullptr),
	memory_ids(std::map<std::string, int>()),
	block_types(std::map<std::string, Type>()),
	variable_types(std::map<std::string, Type>()),
	seed(0)
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
	auto it = memory_ids.find(key);
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

std::string CallExprAST::callee_signature(BuildContext& out)
{
	std::string block_name = this->callee;

	for (auto & elem : Args)
	{
		if (CallExprAST* call_ast = dynamic_cast<CallExprAST*>(elem))
			(elem)->type = out.get_block_type(call_ast->callee_signature(out));
		else if (ArrayIndexAccessExprAST* array_access_ast = dynamic_cast<ArrayIndexAccessExprAST*>(elem))
		{
			Type var_type = out.get_variable_type(array_access_ast->name);
			if (var_type == TypeFactory::get_instance().get("CharAry"))
				var_type = TypeFactory::get_instance().get("Char");
			else if (var_type == TypeFactory::get_instance().get("IntAry"))
				var_type = TypeFactory::get_instance().get("Int");	
			else if (var_type == TypeFactory::get_instance().get("FloatAry"))
				var_type = TypeFactory::get_instance().get("Float");
				
			elem->type = var_type;
		}
		else if (VariableExprAST* var_ast = dynamic_cast<VariableExprAST*>(elem))
			elem->type = out.get_variable_type(var_ast->name);
		else if (VariableFieldExprAST* field_ast = dynamic_cast<VariableFieldExprAST*>(elem))
		{
			auto struct_type = out.get_variable_type(field_ast->name);
			auto field_type = struct_type.get_member(field_ast->field);
			elem->type = field_type;
		}

		block_name += (std::string(":") + (elem)->type.get_name());
	}

	return block_name;
}

int CallExprAST::get_block_id(BuildContext& out, Block* block)
{
 	return out.get_program().get_block_id(callee_signature(out));
}

void ProgramAST::install_types(BuildContext& out)
{
	for (auto & elem : structs)
	{
		std::vector<std::string> fields; 
		std::vector<Type> types;
		for (auto j = (elem)->members.begin(); j != (elem)->members.end(); ++j)
		{
			fields.push_back(j->first);
			types.push_back(TypeFactory::get_instance().get(j->second));
		}

		TypeFactory::get_instance().install(Type((elem)->name, types, fields));
	}
}

void ProgramAST::register_functions(BuildContext& out)
{
	for (auto & elem : functions)
	{
		out.get_program().add_block(new Block((elem)->proto->name));
		out.set_block_type((elem)->proto->name, (elem)->proto->type);		
	}
}

void ProgramAST::emit_bytecode(BuildContext& out)
{
	for (auto & elem : functions)
		(elem)->emit_bytecode(out);
}

ProgramAST::~ProgramAST()
{
	for (auto & elem : structs)
		delete elem;

	for (auto & elem : functions)
		delete elem;
}
