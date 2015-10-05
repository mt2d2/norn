#ifndef PARSER_H
#define PARSER_H

#include <map>

#include "lexer.h"

class ExprAST;
class PrototypeAST;
class FunctionAST;
class StructAST;
class ProgramAST;

class BuildContext;

class Parser {
public:
  Parser(std::istream &stream);
  BuildContext parse(BuildContext &program, int optimize = 0);

private:
  int getNextToken();
  int GetTokPrecedence();
  ExprAST *ParseIdentifierExpr();
  ExprAST *ParseNumberExpr();
  ExprAST *ParseCharExpr();
  ExprAST *ParseStringExpr();
  ExprAST *ParseParenExpr();
  ExprAST *ParsePrimary();
  ExprAST *ParseVMBuiltinExpr();
  ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS);
  ExprAST *ParseExpression();
  ExprAST *ParseIfExpr();
  ExprAST *ParseForExpr();
  ExprAST *ParseWhileExpr();
  ExprAST *ParseReturnExpr();
  ExprAST *ParseBoolExpr();
  PrototypeAST *ParsePrototype();
  FunctionAST *ParseDefinition();
  StructAST *ParseStruct();
  void HandleStruct(ProgramAST &out);
  void HandleDefinition(ProgramAST &out);

  /// Error* - These are little helper functions for error handling.
  ExprAST *Error(const char *str) {
    std::cerr << "Syntax error: " << str << " at line " << lex.get_line()
              << std::endl;
    return (ExprAST *)NULL;
  }

  StructAST *ErrorS(const char *str) {
    Error(str);
    return (StructAST *)NULL;
  }

  PrototypeAST *ErrorP(const char *str) {
    Error(str);
    return (PrototypeAST *)NULL;
  }

  FunctionAST *ErrorF(const char *str) {
    Error(str);
    return (FunctionAST *)NULL;
  }

  Lexer lex;
  int CurToken;
  std::map<char, int> BinopPrecedence;
};

#endif
