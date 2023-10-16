//----------------------------------------------------------------------
// FILE: ast_parser.h
// DATE: CPSC 326, Spring 2023
// AUTH: Nisa Meshal
// DESC: AST parser header file
//----------------------------------------------------------------------

#ifndef AST_PARSER_H
#define AST_PARSER_H

#include "mypl_exception.h"
#include "lexer.h"
#include "ast.h"


class ASTParser
{
public:

  // crate a new recursive descent parer
  ASTParser(const Lexer& lexer);

  // run the parser
  Program parse();
  
private:
  
  Lexer lexer;
  Token curr_token;
  
  // helper functions
  void advance();
  void eat(TokenType t, const std::string& msg);
  bool match(TokenType t);
  bool match(std::initializer_list<TokenType> types);
  void error(const std::string& msg);
  bool bin_op();

  // recursive descent functions
  void struct_def(Program& p);
  void fun_def(Program& s);
  
  void fields(StructDef& s);
  void params(FunDef& f);
  void data_type(DataType& dt);
  bool base_type();
  void stmt(std::vector<std::shared_ptr<Stmt>>& st);
  VarDeclStmt vdecl_stmt();
  VarDeclStmt vdecl_stmt(Token id); 
  void assign_stmt(AssignStmt& s);
  void lvalue (std::vector<VarRef>& v, Token id);
  BasicIf if_stmt();
  IfStmt if_stmt_t(BasicIf bi);
  WhileStmt while_stmt();
  ForStmt for_stmt();
  CallExpr call_expr(Token id);
  ReturnStmt ret_stmt();
  void expr(Expr& e);
  void rvalue(Expr& e);
  NewRValue new_rvalue();
  bool base_rvalue();
  VarRValue var_rvalue(Token id);
};


#endif
