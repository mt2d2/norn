#ifndef TREE_H
#define TREE_H

#include "vm/common.h"
#include "vm/instruction.h"
#include "vm/program.h"
#include "vm/block.h"
#include "vm/opcode.h"
#include "type.h"

// BuildContext
class BuildContext
{
public:
	BuildContext(bool nojit);
	Program& get_program();
	Block* get_block();
	void set_block(Block* block);
	Type get_variable_type(const std::string& key);
	void set_variable_type(const std::string& key, Type value);
	Type get_block_type(const std::string& key);
	void set_block_type(const std::string& key, Type value);
	int get_mem_id(const std::string& key);
	bool variable_exists(const std::string& key);
	int get_and_increment_seed();
	bool get_nojit() const;

private:
	Program program;
	Block* working_block;
	std::map<std::string, int> memory_ids;
	std::map<std::string, Type> block_types;
	std::map<std::string, Type> variable_types;
	int seed;
	bool nojit;
};

/// ExprAST - Base class for all expression nodes.
class ExprAST
{
public:
	ExprAST() : type(TypeFactory::get_instance().get(VOID)) { }
	virtual ~ExprAST() {}
	virtual void emit_bytecode(BuildContext& out) = 0;
	Type type;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST
{
public:
	NumberExprAST(double v, bool is_float) : val(v)
	{
		type = is_float ? TypeFactory::get_instance().get(FLOAT) : TypeFactory::get_instance().get(INT);
	}

	virtual void emit_bytecode(BuildContext& out);
	virtual ~NumberExprAST()
	{
	}

private:
	double val;
};

class CharExprAST : public ExprAST
{
public:
	CharExprAST(char c) : val(c) { type = TypeFactory::get_instance().get(CHAR); }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~CharExprAST()
	{
	}

private:
	char val;
};

class StringExprAST : public ExprAST
{
public:
	StringExprAST(std::string s) : val(s) { type = TypeFactory::get_instance().get(CHAR_ARY); }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~StringExprAST()
	{
	}

private:
	std::string val;
};

class VMBuiltinExprAST : public ExprAST
{
public:
	VMBuiltinExprAST(Opcode op) : op(op) { type = TypeFactory::get_instance().get(VOID); }
	virtual void emit_bytecode(BuildContext& out);
  	virtual ~VMBuiltinExprAST()
	{
	}

	Opcode op;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST
{
public:
	VariableExprAST(const std::string &n) : name(n) { }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~VariableExprAST() { }
	std::string name;
};

class VariableAssignExprAST : public ExprAST
{
public:
	VariableAssignExprAST(const std::string& name, ExprAST* value) : name(name), value(value) { }
	VariableAssignExprAST(const std::string& name, ExprAST* value, std::string type) : name(name), value(value) { this->type = TypeFactory::get_instance().get(type); }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~VariableAssignExprAST() { }
	std::string name;
	ExprAST* value;
};

class VariableAssignShortExprAST : public VariableAssignExprAST
{
public:
	VariableAssignShortExprAST(const std::string& name, ExprAST* value) : VariableAssignExprAST(name, value) { }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~VariableAssignShortExprAST() { }
};

class ArrayDeclarationExprAST : public ExprAST
{
public:
	ArrayDeclarationExprAST(std::string id, std::string type, int size) : name(id), size(size) 
	{ 
		if (type == "Int")
			this->type = TypeFactory::get_instance().get(INT_ARY);
		else if (type == "Char")
			this->type = TypeFactory::get_instance().get(CHAR_ARY);
		else if (type == "Float")
			this->type = TypeFactory::get_instance().get(FLOAT_ARY);
	}

	virtual void emit_bytecode(BuildContext& out);
	virtual ~ArrayDeclarationExprAST()
	{
	}

private:
	std::string name;
	int size;
};

class ArrayIndexAssignExprAST : public ExprAST
{
public:
	ArrayIndexAssignExprAST(std::string id, ExprAST* index, ExprAST* assignee) : name(id), index(index), value(assignee)
	{
		this->type = value->type;
	}
	virtual void emit_bytecode(BuildContext& out);
	virtual ~ArrayIndexAssignExprAST()
	{
	}

private:
	std::string name;
	ExprAST* index;
	ExprAST* value;
};

class ArrayIndexAccessExprAST : public VariableExprAST
{
public:
	ArrayIndexAccessExprAST(const std::string& id, ExprAST* index) :  VariableExprAST(id), index(index) 
	{ 
	}
	virtual void emit_bytecode(BuildContext& out);
	virtual ~ArrayIndexAccessExprAST()
	{
	}

private:
	ExprAST* index;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST
{
public:
	BinaryExprAST(char o, ExprAST* lhs, ExprAST *rhs) : op(o), left(lhs), right(rhs)
	{
		if (left->type == TypeFactory::get_instance().get(FLOAT) || right->type == TypeFactory::get_instance().get(FLOAT))
			this->type = TypeFactory::get_instance().get(FLOAT);
		else
			this->type = TypeFactory::get_instance().get(INT);
	}
	virtual void emit_bytecode(BuildContext& out);
	virtual ~BinaryExprAST()
	{
		delete left;
		delete right;
	}

private:
	char op;
	ExprAST* left;
	ExprAST* right;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST
{
public:
	CallExprAST(const std::string &c, std::vector<ExprAST*>& args) : Args(args), callee(c) { type = TypeFactory::get_instance().get(VOID); }
	virtual void emit_bytecode(BuildContext& out);
	int get_block_id(BuildContext& out, Block* block);
	std::string callee_signature(BuildContext& out);
	Type resolve_return_type(BuildContext& out);
	virtual ~CallExprAST()
	{
		for (std::vector<ExprAST*>::iterator i = Args.begin(); i != Args.end(); ++i)
			delete *i;
	}

private:
	std::vector<ExprAST*> Args;
	std::string callee;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST
{
public:
	IfExprAST(ExprAST* c) : cond(c), then(std::vector<ExprAST*>()), otherwise(std::vector<ExprAST*>()) { type = TypeFactory::get_instance().get(VOID); }
	void add_then_expression(ExprAST* e) { then.push_back(e); }
	void add_otherwise_expression(ExprAST* e) { otherwise.push_back(e); }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~IfExprAST()
	{
		delete cond;
		for (std::vector<ExprAST*>::iterator i = then.begin(); i != then.end(); ++i)
			delete *i;
		for (std::vector<ExprAST*>::iterator i = otherwise.begin(); i != otherwise.end(); ++i)
			delete *i;
	}

private:
	ExprAST* cond;
	std::vector<ExprAST*> then;
	std::vector<ExprAST*> otherwise; // else is reserved, and I hate ASCII vomit
};

/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST
{
public:
	ForExprAST(const std::string &varname, ExprAST *start, ExprAST *end, ExprAST *step) : VarName(varname), Start(start), End(end), Step(step), body(std::vector<ExprAST*>()) { type = TypeFactory::get_instance().get(VOID); }
	void add_expression(ExprAST* e) { body.push_back(e); }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~ForExprAST()
	{
		delete Start;
		delete End;
		delete Step;
		for (std::vector<ExprAST*>::iterator i = body.begin(); i != body.end(); ++i)
			delete *i;
	}

private:
	std::string VarName;
	ExprAST *Start, *End, *Step;
	std::vector<ExprAST*> body;
};

/// WhileExprAST - Expression class for for/in.
class WhileExprAST : public ExprAST
{
public:
	WhileExprAST(ExprAST* cond) : cond(cond), body(std::vector<ExprAST*>()) { }
	void add_expression(ExprAST* e) { body.push_back(e); }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~WhileExprAST()
	{
		delete cond;
		for (std::vector<ExprAST*>::const_iterator i = body.begin(); i != body.end(); ++i)
			delete *i;
	}

private:
	ExprAST* cond;
	std::vector<ExprAST*> body;
};

/// ReturnExprAST - Expression class for return
class ReturnExprAST : public ExprAST
{
public:
	ReturnExprAST(ExprAST* val) : val(val) { }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~ReturnExprAST() { delete val; }

private:
	ExprAST* val;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST
{
public:
	PrototypeAST(const std::string &n, const std::map<std::string, std::string> &a, const std::string& ftype, bool needs_jit) : args(std::map<std::string, Type>()), name(n), type(TypeFactory::get_instance().get(ftype)), needs_jit(needs_jit)
	{
		for (std::map<std::string, std::string>::const_iterator i = a.begin(); i != a.end(); ++i)
			args[i->first] = TypeFactory::get_instance().get(i->second);

		name = n;
		for (std::map<std::string, Type>::iterator i = args.begin(); i != args.end(); ++i)
			name += (std::string(":") + (*i).second.get_name());
	}
	virtual void emit_bytecode(BuildContext& out);
	virtual ~PrototypeAST()
	{
	}

	std::map<std::string, Type> args;
	std::string name;
	Type type;
	bool needs_jit;
};

class StructAST
{
public:
	StructAST(const std::string& name, std::map<std::string, std::string> members) : name(name), members(members) { }

	std::string name;
	std::map<std::string, std::string> members;
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST
{
public:
	FunctionAST(PrototypeAST* p) : proto(p), body(std::vector<ExprAST*>()) { }
	void add_expression(ExprAST* e) { body.push_back(e); }
	virtual void emit_bytecode(BuildContext& out);
	virtual ~FunctionAST()
	{
		delete proto;
		for (std::vector<ExprAST*>::const_iterator i = body.begin(); i != body.end(); ++i)
			delete *i;
	}

	PrototypeAST* proto;
	std::vector<ExprAST*> body;
};

class ProgramAST
{
public:
	ProgramAST() : functions(std::vector<FunctionAST*>()), structs(std::vector<StructAST*>()) { };
	virtual ~ProgramAST();
	void add_struct(StructAST* s) { structs.push_back(s); }
	void add_function(FunctionAST* f) { functions.push_back(f); }
	void install_types(BuildContext& out);
	void register_functions(BuildContext& out);
	void emit_bytecode(BuildContext& out);

private:
	std::vector<StructAST*> structs;
	std::vector<FunctionAST*> functions;
};

#endif
