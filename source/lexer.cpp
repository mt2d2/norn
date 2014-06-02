#include "lexer.h"

#include <cctype>  // isdigit, isspace
#include <cstdlib> // strtod

#include "vm/common.h"

Lexer::Lexer(std::istream &stream)
    : source(stream), pos(0), line(1), identifier(std::string()),
      floater(false), number(0.0) {}

int Lexer::get_token() {
  char current = source[pos];
  if (current == '\n')
    ++line;

whitespace:
  while (isspace(current)) {
    current = source[++pos];
    if (current == '\n')
      ++line;
  }

  if (current == '#') {
    // skip to end
    while (current != '\r' && current != '\n')
      current = source[++pos];

    goto whitespace;
  }

  if (current == '$') {
    // eat $
    current = source[++pos];

    this->identifier = current;
    while (isalnum(current = source[++pos]))
      this->identifier += current;

    return tok_vm_builtin;
  }

  if (current == '&' && source[pos + 1] == '&') {
    pos += 2;
    current = source[pos];
    return 'a';
  }

  if (current == '|' && source[pos + 1] == '|') {
    pos += 2;
    current = source[pos];
    return 'o';
  }

  if (current == '<' && source[pos + 1] == '=') {
    pos += 2;
    current = source[pos];
    return 'l';
  }

  if (current == '>' && source[pos + 1] == '=') {
    pos += 2;
    current = source[pos];
    return 'g';
  }

  if (current == '!' && source[pos + 1] == '=') {
    pos += 2;
    current = source[pos];
    return '!';
  }

  if (current == '\'') {
    current = source[++pos]; // eat '
    // capture char, handle escape sequences
    this->character = current;
    if (this->character == '\\') {
      current = source[++pos]; // eat escape
      this->character = lex_escape(current);
      current = source[++pos]; // eat escape sequence
    } else {
      current = source[++pos]; // eat char
    }

    current = source[++pos]; // eat '

    return tok_char;
  }

  if (current == '"') {
    this->string = "";

    current = source[++pos]; // eat "
    while (current != '"') {
      if (current == '\\')
        this->string += lex_escape(source[++pos]);
      else
        this->string += current;

      current = source[++pos]; // eat char
    }
    current = source[++pos]; // eat "

    return tok_string;
  }

  if (isalpha(current) || current == '_') {
    this->identifier = current;

    while (isalnum(current = source[++pos]) || current == '_')
      this->identifier += current;

    if (this->identifier == "def")
      return tok_def;
    else if (this->identifier == "end")
      return tok_end;
    else if (this->identifier == "struct")
      return tok_struct;
    else if (this->identifier == "if")
      return tok_if;
    else if (this->identifier == "then")
      return tok_then;
    else if (this->identifier == "else")
      return tok_else;
    else if (this->identifier == "for")
      return tok_for;
    else if (this->identifier == "while")
      return tok_while;
    else if (this->identifier == "in")
      return tok_in;
    else if (this->identifier == "return")
      return tok_return;
    else if (this->identifier == "true" || this->identifier == "false") {
      this->boolean = (this->identifier == "true") ? true : false;
      return tok_bool;
    } else
      return tok_identifier;
  }

  bool is_negative = (current == '-');
  if (is_negative && isdigit(source[pos + 1]))
    current = source[++pos];

  if (isdigit(current)) {
    std::string num;
    num += current;
    current = source[++pos];
    while (isdigit(current) || current == '.') {
      floater = floater || (current == '.');
      num += current;
      current = source[++pos];
    }

    this->number = strtod(num.c_str(), nullptr);
    this->number = is_negative ? -this->number : this->number;

    return tok_number;
  }

  if (current == 0)
    return tok_eof;

  pos++;
  return current;
}

int Lexer::get_line() { return line; }

std::string Lexer::get_identifier() { return identifier; }

bool Lexer::is_float() {
  bool ret = floater;
  floater = false;
  return ret;
}

double Lexer::get_number() { return number; }

char Lexer::get_char() { return character; }

bool Lexer::get_bool() { return boolean; }

const std::string &Lexer::get_string() { return string; }

char Lexer::lex_escape(char escape) {
  char c = '\0';

  switch (escape) {
  case 'a':
    c = '\a';
    break;
  case 'b':
    c = '\b';
    break;
  case 'f':
    c = '\f';
    break;
  case 'n':
    c = '\n';
    break;
  case 'r':
    c = '\r';
    break;
  case 't':
    c = '\t';
    break;
  case 'v':
    c = '\v';
    break;
  default:
    raise_error("unhandled escape");
    break;
  }

  return c;
}
