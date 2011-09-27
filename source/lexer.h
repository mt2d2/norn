#ifndef LEXER_H
#define LEXER_H

#include <iostream>
#include <fstream>

class FileReader
{
public:
	FileReader(std::istream& stream) :
		is(stream)
	{
	}
		
	char operator[](size_t i)
	{ 
		this->is.seekg(i);
		return this->is.get();
	}
	
private:
	std::istream& is;
};

enum Token
{
	tok_eof = -358,

	// commands
	tok_def, tok_end, tok_struct,

	// primary
	tok_identifier, tok_type, tok_number, tok_char, tok_string, tok_bool,

	// control
	tok_if, tok_then, tok_else, tok_for, tok_while, tok_in, tok_return,

	// builtin
	tok_vm_builtin
};

class Lexer
{
public:
	Lexer(std::istream& stream);
	int get_token();
	int get_line();
	std::string get_identifier();
	bool is_float();
	double get_number();
	char get_char();
	bool get_bool();
	std::string& get_string();

private:
	char lex_escape(char escape);

	FileReader source;
	unsigned int pos;
	int line;

	std::string identifier;
	bool floater;
	double number;
	char character;
	bool boolean;
	std::string string;
};

#endif

