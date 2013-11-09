#include "parser.h"

Parser::Parser(std::istream& stream) : lex(Lexer(stream)), CurToken(0), BinopPrecedence(std::map<char, int>())
{
	// Install standard binary operators.
	// 1 is lowest precedence.
	BinopPrecedence['o'] = 5;
	BinopPrecedence['a'] = 5; 
	BinopPrecedence['<'] = 10;
	BinopPrecedence['l'] = 10;
	BinopPrecedence['>'] = 10;
	BinopPrecedence['g'] = 10;
	BinopPrecedence['='] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['/'] = 40;
	BinopPrecedence['%'] = 40;
	BinopPrecedence['*'] = 40;
	BinopPrecedence['&'] = 40;
	BinopPrecedence['|'] = 40; // highest.
}

int Parser::getNextToken()
{
    return CurToken = lex.get_token();
}

int Parser::GetTokPrecedence()
{
    if (!isascii(CurToken))
        return -1;

    // Make sure it's a declared binop.
    int TokPrec = BinopPrecedence[CurToken];
    if (TokPrec <= 0)
        return -1;

    return TokPrec;
}

ExprAST* Parser::ParseIdentifierExpr()
{
    std::string IdName = lex.get_identifier();
	std::string var_type;

    getNextToken();  // eat identifier.

	if (CurToken == '=')
	{
		getNextToken();
		if (ExprAST* e = ParseExpression())
			return new VariableAssignShortExprAST(IdName, e);
		else
			return Error("expecting expression after variable assign");
	}

	if (CurToken == ':')
	{
		getNextToken(); // eat :

		if (CurToken == '[')
		{
			getNextToken(); // eat [

			// parsing array type
			if (CurToken != tok_number)
				return Error("expecting 'tok_number' in array declaration\n");

			int array_size = lex.get_number();
			getNextToken(); // eat number

			if (CurToken != ']')
				return Error("expecting ']' after array declaration\n");
			getNextToken(); // eat ]

			var_type = lex.get_identifier();
			getNextToken(); // eat type

			return new ArrayDeclarationExprAST(IdName, var_type, array_size);
		}
		else
		{
			// normal variable type
			var_type = lex.get_identifier();
			getNextToken(); // eat type
		}

		if (CurToken == '=')
		{
			getNextToken(); // eat =
			if (ExprAST* expr = ParseExpression())
				return new VariableAssignExprAST(IdName, expr, var_type);
			else
				return nullptr;
		}
	}

	if (CurToken == '[')
	{
		getNextToken(); // eat [
		ExprAST* index = ParseExpression();
		if (!index)
			return Error("expecting primary for index  after array access\n");

		if (CurToken != ']')
			return Error("expecting closing \']\' of array access\n");
		getNextToken(); // eat ]

		if (CurToken == '=')
		{
			getNextToken(); // eat =
			ExprAST* assignee = ParsePrimary();

			return new ArrayIndexAssignExprAST(IdName, index, assignee);
		}
		else
		{
			// return some kind of just access
			return new ArrayIndexAccessExprAST(IdName, index);
		}
	}

    if (CurToken != '(') // Simple variable ref.
	    return new VariableExprAST(IdName);

    // Call.
    getNextToken();  // eat (
    std::vector<ExprAST*> Args;
    if (CurToken != ')')
    {
        while (1)
        {
            ExprAST* Arg = ParseExpression();

            if (!Arg)
                return nullptr;
			else
            	Args.push_back(Arg);

            if (CurToken == ')')
              break;

            if (CurToken != ',')
                return Error("Expected ')' or ',' in argument list");

            getNextToken();
        }
    }

    // Eat the ')'.
    getNextToken();

    return new CallExprAST(IdName, Args);
}

ExprAST* Parser::ParseNumberExpr()
{
    ExprAST* ret = new NumberExprAST(lex.get_number(), lex.is_float());
    getNextToken(); // consume the number
    return ret;
}

ExprAST* Parser::ParseCharExpr()
{
	ExprAST* ret = new CharExprAST(lex.get_char());
	getNextToken(); // consume the char: e.g., 'a'
	return ret;
}

ExprAST* Parser::ParseStringExpr()
{
	ExprAST* ret = new StringExprAST(lex.get_string());
	getNextToken(); // consume the char: e.g., 'a'
	return ret;
}

ExprAST* Parser::ParseParenExpr()
{
    // eat (.
    getNextToken();

    ExprAST* V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurToken != ')')
        return Error("expected ')'");

    // eat ).
    getNextToken();

    return V;
}

ExprAST* Parser::ParsePrimary()
{
    switch (CurToken)
    {
        case tok_identifier:
            return ParseIdentifierExpr();
            break;
        case tok_number:
            return ParseNumberExpr();
            break;
		case tok_char:
			return ParseCharExpr();
			break;
		case tok_string:
			return ParseStringExpr();
			break;
        case '(': 
            return ParseParenExpr();
            break;
		case tok_if:
			return ParseIfExpr();
			break;
		case tok_for:
			return ParseForExpr();
			break;
		case tok_while:
			return ParseWhileExpr();
			break;
		case tok_vm_builtin:
			return ParseVMBuiltinExpr();
			break;
		case tok_return:
			return ParseReturnExpr();
			break;
		case tok_bool:
			return ParseBoolExpr();
			break;
        default:
			std::cout << "Unknown token: '" << (char)CurToken << "' (int)" << CurToken << std::endl;
			return Error("unknown token when expecting an expression.");
            break;
    }
}

ExprAST* Parser::ParseVMBuiltinExpr()
{
	Opcode which = RTRN;
	if (lex.get_identifier() == "PrintInt")
		which = PRINT_INT;
	else if (lex.get_identifier() == "PrintChar")
		which = PRINT_CHAR;
	else if (lex.get_identifier() == "PrintFloat")
		which = PRINT_FLOAT;
	else if (lex.get_identifier() == "PrintCharArray")
		which = PRINT_ARY_CHAR;

	getNextToken(); // eat VM Token
	return new VMBuiltinExprAST(which);
}

ExprAST* Parser::ParseBinOpRHS(int ExprPrec, ExprAST* LHS)
{
    // If this is a binop, find its precedence.
    while (1)
    {
        int TokPrec = GetTokPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < ExprPrec)
            return LHS;

        // Okay, we know this is a binop.
        int BinOp = CurToken;
        getNextToken();  // eat binop

        // Parse the primary expression after the binary operator.
        ExprAST *RHS = ParsePrimary();
        if (!RHS) return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec)
        {
            RHS = ParseBinOpRHS(TokPrec+1, RHS);
            if (!RHS)
				return nullptr;
        }

        // Merge LHS/RHS.
        LHS = new BinaryExprAST(BinOp, LHS, RHS);
    }
}

ExprAST* Parser::ParseExpression()
{
    ExprAST *LHS = ParsePrimary();
    if (!LHS)
      return nullptr;

    return ParseBinOpRHS(0, LHS);
}

PrototypeAST* Parser::ParsePrototype()
{
    if (CurToken != tok_identifier)
        return ErrorP("Expected function name in prototype");

    std::string FnName = lex.get_identifier();
    getNextToken();
  
    if (CurToken != '(')
        return ErrorP("Expected '(' in prototype");
	else
		getNextToken();
  
    std::map<std::string, std::string> args;
    while (CurToken == tok_identifier)
	{
        std::string var_name = lex.get_identifier();
		getNextToken();
					
		if (CurToken != ':')
			return ErrorP("Expected ':' before type information in prototype.");
		else 
			getNextToken(); // eat :
	
		std::string var_type = lex.get_identifier();
		args[var_name] = var_type;	
				
		getNextToken(); // eat var type
			
		if (CurToken == ',')
			getNextToken();
	}
	
    if (CurToken != ')')
        return ErrorP("Expected ')' in prototype");
  
    // success.
    getNextToken();  // eat ')'.

	if (CurToken != ':')
		return ErrorP("Expected ':' before function type information");
	else
		getNextToken();
	
	std::string function_type;
	if (CurToken != tok_identifier)
		return ErrorP("Expected type information for function");
  	else
	{
		function_type = lex.get_identifier();
		getNextToken();
	}
		
    return new PrototypeAST(FnName, args, function_type);
}

FunctionAST* Parser::ParseDefinition()
{
    getNextToken();  // eat def

    PrototypeAST *Proto = ParsePrototype();
    if (Proto == nullptr)
        return nullptr;

	auto  function_ast = new FunctionAST(Proto);

	while (CurToken != tok_end)
	{
    	if (ExprAST *E = ParseExpression())
			function_ast->add_expression(E);
		else if (CurToken != tok_end)
			return ErrorF("Expected 'end' at the end of a function.");
    }

	getNextToken(); // eat end
	
	return function_ast;
}

ExprAST* Parser::ParseIfExpr() 
{
	getNextToken();  // eat the if

	ExprAST* cond = ParseExpression();	
	if (!cond)
		return Error("Expected expression after 'if'");

	if (CurToken != tok_then)
		return Error("expected 'then' after the condition of 'if'");
	getNextToken();  // eat 'in'.

	auto  if_ast = new IfExprAST(cond);
	while (CurToken != tok_else && CurToken != tok_end)
	{
    	if (ExprAST *E = ParseExpression())
			if_ast->add_then_expression(E);
		else 
			return Error("unable to parse expression in 'if' statment");
    }

	if (CurToken == tok_end)
	{
		getNextToken(); // eat end
		return if_ast;
	}

	if (CurToken != tok_else)
		return Error("expected 'else' or 'end' after 'if'");
	else
		getNextToken(); // eat else

	while (CurToken != tok_end)
	{
		if (ExprAST* e = ParseExpression())
			if_ast->add_otherwise_expression(e);
		else if (CurToken != tok_end)
			return Error("expected 'else' or 'end' after 'if'");
	}
	getNextToken(); // eat end

	return if_ast;
}

ExprAST* Parser::ParseForExpr()
{
	getNextToken();  // eat the for.

	if (CurToken != tok_identifier)
		return Error("expected identifier after for");

	std::string IdName = lex.get_identifier();
	ExprAST* Start = ParseExpression();
	if (!Start)
		return nullptr;
	if (CurToken != ',')
		return Error("expected ',' after for start value");
	getNextToken();

	ExprAST *End = ParseExpression();
	if (!End)
		return nullptr;

	ExprAST *Step = nullptr;
	if (CurToken == ',')
	{
		getNextToken();
		Step = ParseExpression();
		if (!Step)
			return nullptr;
	}

	if (CurToken != tok_in)
		return Error("expected 'in' at the end of a 'for' statment");
	getNextToken();  // eat 'in'.

	auto  for_ast = new ForExprAST(IdName, Start, End, Step);
	while (CurToken != tok_end)
	{
    	if (ExprAST *E = ParseExpression())
			for_ast->add_expression(E);
		else if (CurToken != tok_end)
			return Error("Expected 'end' at the end of a 'for' block");
    }
	getNextToken(); // eat end

	return for_ast;
}


ExprAST* Parser::ParseWhileExpr()
{
	getNextToken();  // eat the while

	ExprAST* cond = ParseExpression();
	if (!cond)
		return Error("Expected expression after while");

	if (CurToken != tok_in)
		return Error("expected 'in' after while");
	getNextToken();  // eat 'in'.

	auto  while_ast = new WhileExprAST(cond);
	while (CurToken != tok_end)
	{
    	if (ExprAST *E = ParseExpression())
			while_ast->add_expression(E);
		else if (CurToken != tok_end)
			return Error("Expected 'end' at the end of a 'while' block");
    }
	getNextToken(); // eat end

	return while_ast;
}

ExprAST* Parser::ParseReturnExpr()
{
	getNextToken(); // eat return
	ExprAST* val = ParseExpression();
	if (!val)
		return Error("expected expression after 'return'");
	else
		return new ReturnExprAST(val);
}

ExprAST* Parser::ParseBoolExpr()
{
    ExprAST* ret = new NumberExprAST(lex.get_bool() ? 1 : 0, false);
	getNextToken(); // eat bool
	return ret;
}

StructAST* Parser::ParseStruct()
{
	getNextToken(); // eat struct

	if (CurToken != tok_identifier)
		return ErrorS("expected identifer after 'struct'");
	
	std::string identifier = lex.get_identifier();
	getNextToken(); // eat identifer
	

	std::map<std::string, std::string> members;
	while (CurToken != tok_end)
	{
		if (CurToken != tok_identifier)
			return ErrorS("expected identifer before type information");
		std::string identifier = lex.get_identifier();
		getNextToken(); // eat identifier

		if (CurToken != ':')
			return ErrorS("expected identifer before type information");
		getNextToken(); // eat :

		if (CurToken != tok_identifier)
			return ErrorS("expected identifer before type information");
		std::string type = lex.get_identifier();
		getNextToken(); // eat identifier

		members[identifier] = type;
	}	

	if (CurToken != tok_end)
		return ErrorS("expected 'end' at end of struct block");
	getNextToken(); // eat end

	return new StructAST(identifier, members);
}

void Parser::HandleStruct(ProgramAST& out)
{
	if (StructAST* s = ParseStruct())
	{
		out.add_struct(s);
	} 
	else 
	{
		// Skip token for error recovery
		getNextToken();
	}
}

void Parser::HandleDefinition(ProgramAST& out) 
{
    if (FunctionAST* func = ParseDefinition()) 
    {
		out.add_function(func);
 	} 
    else
    {
        // Skip token for error recovery.
        getNextToken();
    } 
}

BuildContext Parser::parse(BuildContext& build, int optimize) 
{
	ProgramAST ast;
	getNextToken();
	while (CurToken == tok_struct || CurToken == tok_def)
	{
		if (CurToken == tok_struct)
		{
			HandleStruct(ast);
			ast.install_types(build);
		}
		else
		{
			HandleDefinition(ast);
			ast.register_functions(build);
		}
	}

	ast.emit_bytecode(build);

	if (optimize >= 1)
	{
		build.get_program().store_load_elimination();
		build.get_program().fold_constants();
		// build.get_program().inline_calls();
		build.get_program().lit_load_add();
		build.get_program().lit_load_sub();
		build.get_program().lit_load_le();
	}
	
	return build;
}
