//----------------------------------------------------------------------
// FILE: simple_parser.cpp
// DATE: CPSC 326, Spring 2023
// AUTH:
// DESC:
//----------------------------------------------------------------------

#include "simple_parser.h"
#include <iostream>
using namespace std; 

SimpleParser::SimpleParser(const Lexer& a_lexer)
  : lexer {a_lexer}
{}


void SimpleParser::advance()
{
  curr_token = lexer.next_token();
}


void SimpleParser::eat(TokenType t, const std::string& msg)
{
  if (!match(t))
    error(msg);
  advance();
}


bool SimpleParser::match(TokenType t)
{
  return curr_token.type() == t;
}


bool SimpleParser::match(std::initializer_list<TokenType> types)
{
  for (auto t : types)
    if (match(t))
      return true;
  return false;
}


void SimpleParser::error(const std::string& msg)
{
  std::string s = msg + " found '" + curr_token.lexeme() + "' ";
  s += "at line " + std::to_string(curr_token.line()) + ", ";
  s += "column " + std::to_string(curr_token.column());
  throw MyPLException::ParserError(s);
}


bool SimpleParser::bin_op()
{
  return match({TokenType::PLUS, TokenType::MINUS, TokenType::TIMES,
      TokenType::DIVIDE, TokenType::AND, TokenType::OR, TokenType::EQUAL,
      TokenType::LESS, TokenType::GREATER, TokenType::LESS_EQ,
      TokenType::GREATER_EQ, TokenType::NOT_EQUAL});
}


void SimpleParser::parse()
{
  advance();
  while (!match(TokenType::EOS)) {
    if (match(TokenType::STRUCT))
      struct_def();
    else
      fun_def();
  }
  eat(TokenType::EOS, "expecting end-of-file");
}


void SimpleParser::struct_def()
{
  eat(TokenType::STRUCT, "expecting struct in struct def");
  eat(TokenType::ID, "expecting id in struct def");

  eat(TokenType::LBRACE, "expecting l brace in struct def");
  if(match(TokenType::RBRACE))
  {
    eat(TokenType::RBRACE, "expecting r brace for empty struct");
  }
  else 
  {
    fields();
    eat(TokenType::RBRACE, "expecting r brace in struct def");
  }
}

void SimpleParser::fields() 
{
  data_type();
  eat(TokenType::ID, "expected an ID for filds");
  if(match(TokenType::COMMA))
  {
    while(match(TokenType::COMMA))
    {
      eat(TokenType::COMMA, "expecting comma in fields");
      data_type();
      eat(TokenType::ID, "expecting id in fields");
    }
  }
}

void SimpleParser::fun_def()
{
  //compare against void or a data type
  if(match(TokenType::VOID_TYPE))
  {
    eat(TokenType::VOID_TYPE, "expecting void in fun def");
  }
  else 
  {
    data_type();
  }

  eat(TokenType::ID, "expecting function name");
  eat(TokenType::LPAREN, "expecting lparen");
  params();
  eat(TokenType::RPAREN, "expecting rparen");

  eat(TokenType::LBRACE, "expecting lbrace");
  while(!match(TokenType::RBRACE))
  {
    stmt();
  }
  eat(TokenType::RBRACE, "expecting rbrace");
}

void SimpleParser::params() 
{
  //read until you hit rparen
  while(!match(TokenType::RPAREN)) 
  {
    data_type();
    eat(TokenType::ID, "expected an ID");
    //if you match with a comma, then move curr_token forward and call fields
    if(match(TokenType::COMMA))
    {
      eat(TokenType::COMMA, "expecting comma in params");
      fields();
    }
  }
}

//read in data types
void SimpleParser::data_type()
{
  if(match(TokenType::ID))
  {
    eat(TokenType::ID, "expecting id in data type");
  }
  else if (match(TokenType::ARRAY))
  {
    eat(TokenType::ARRAY, "expecting array in data type");
    if(base_type())
    {
      advance();
    }
    else{
      eat(TokenType::ID, "expecting id in array data type");
    }
  }
  else 
  {
    if(base_type())
    {
      advance();
    }
  }
}

//returns true if any of the base type tokens were matched
bool SimpleParser::base_type() 
{
  return match({TokenType::STRING_TYPE, TokenType::DOUBLE_TYPE, TokenType::INT_TYPE, TokenType::BOOL_TYPE, TokenType::CHAR_TYPE});
}

void SimpleParser::stmt() 
{
  if(match(TokenType::ID))
  {
    eat(TokenType::ID, "expecting id in stmt");
    if(match(TokenType::LPAREN))
    {
      call_expr();
    }
    else
    {
      assign_stmt();
    }
  }
  else if (match(TokenType::RETURN))
  {
    eat(TokenType::RETURN, "expecting return in stmt"); 
    ret_stmt();
  }
  else if (match(TokenType::IF))
  {
    eat(TokenType::IF, "expecting if in stmt");
    if_stmt();
  }
  else if (match(TokenType::WHILE))
  {
    eat(TokenType::WHILE, "expecting while in stmt");
    while_stmt();
  }
  else if (match(TokenType::FOR))
  {
    eat(TokenType::FOR, "expecting for in stmt");
    for_stmt();
  }
  else
  {
    vdecl_stmt();
  }
}

void SimpleParser::vdecl_stmt() 
{
  data_type();
  eat(TokenType::ID, "expecting variable ID in vdecl");
  eat(TokenType::ASSIGN, "expecting assignment");
  expr();
}

void SimpleParser::assign_stmt()
{
  if(match(TokenType::ID))
  {
    eat(TokenType::ID, "expectign id in assign"); 
  }
  lvalue();
  eat(TokenType::ASSIGN, "expecting assign");
  expr();
}

void SimpleParser::lvalue()
{
  //eat(TokenType::ID, "expecting id in lvalue");

  while(match(TokenType::DOT) || match(TokenType::LBRACKET))
  {
    if(match(TokenType::DOT))
    {
      eat(TokenType::DOT, "expecting dot in l value");
      eat(TokenType::ID, "expecting id");
    }
    else if(match(TokenType::LBRACKET))
    { 
      eat(TokenType::LBRACKET, "expecting l bracket in l value");
      expr();
 
      //wer are sitting at ']' now, but need to check if it's going to be 2d array
      if(lexer.peek() == '[') {
        eat(TokenType::RBRACKET, "expecting r bracket in l value");
        eat(TokenType::LBRACKET, "expecting l bracket in l value inside");
        expr();
        eat(TokenType::RBRACKET, "expecting r bracket in l value");
      }
      else { //1d array
        eat(TokenType::RBRACKET, "expecting r bracket in lvalue");
      }
    }
  }
}

void SimpleParser::if_stmt()
{
  eat(TokenType::LPAREN, "expecting l paren");
  expr();
  eat(TokenType::RPAREN, "expecting r paren");
  eat(TokenType::LBRACE, "expecting l brace");
  while(!match(TokenType::RBRACE))
  {
    stmt();
  }
  eat(TokenType::RBRACE, "expecting r brace");
  if_stmt_t();
}

void SimpleParser::if_stmt_t()
{
  if(match(TokenType::ELSEIF))
  {
    eat(TokenType::ELSEIF, "expecting else if in if tail");
    eat(TokenType::LPAREN, "expecting l paren");
    expr();
    eat(TokenType::RPAREN, "expecting r paren");
    eat(TokenType::LBRACE, "expecting l brace");
    while(!match(TokenType::RBRACE))
    {
      stmt();
    }
    eat(TokenType::RBRACE, "expecting r brace");
    if_stmt_t();
  }
  else if(match(TokenType::ELSE))
  {
    eat(TokenType::ELSE, "expecting else in if tail");
    eat(TokenType::LBRACE, "expecting l brace");
    while(!match(TokenType::RBRACE))
    {
      stmt();
    }
    eat(TokenType::RBRACE, "expecting r brace");
  }
}

void SimpleParser::while_stmt()
{
  eat(TokenType::LPAREN,"expecting l paren");
  expr();
  eat(TokenType::RPAREN, "expecting r paren");
  eat(TokenType::LBRACE, "expecting l brace");
  while(!match(TokenType::RBRACE))
  {
    stmt();
  }
  eat(TokenType::RBRACE, "expecting r brace");
}

void SimpleParser::for_stmt()
{
  eat(TokenType::LPAREN, "expecting l paren");
  vdecl_stmt();
  eat(TokenType::SEMICOLON, "expecting ;");

  expr();
  eat(TokenType::SEMICOLON, "expecting ;");

  eat(TokenType::ID, "expecting id in for stmt"); ///?????
  assign_stmt();
  eat(TokenType::RPAREN, "expecting r paren in for");
  eat(TokenType::LBRACE, "expecting l brace in for");
  while(!match(TokenType::RBRACE))
  {
    stmt();
  }
  eat(TokenType::RBRACE, "expecting r brace");
}

void SimpleParser::call_expr()
{
  eat(TokenType::LPAREN, "expecting l paren in call expr");
  if(match(TokenType::RPAREN)) //if curr token is r paren, the parens are empty
  {
  
  }
  else 
  {
    expr();
    while(match(TokenType::COMMA))
    {
      eat(TokenType::COMMA, "expecting comma in call expr");
      expr(); 
    }
  }
  eat(TokenType::RPAREN, "expecting r paren");
}

void SimpleParser::ret_stmt()
{
  expr();
}

void SimpleParser::expr()
{
  if (match(TokenType::NOT))
  {
    eat(TokenType::NOT, "expecting not in expr");
    expr();
  } 
  else if (match(TokenType::LPAREN))
  {
    eat(TokenType::LPAREN, "expecting l paren in expr");
    expr();
    eat(TokenType::RPAREN, "expecting r paren in expr");
  }
  else 
  {
    rvalue();
  }

  if(bin_op())
  {
    advance();
    expr();
  }
}

void SimpleParser::rvalue()
{
  if(base_rvalue())
  {
    advance();
  }
  else if (match(TokenType::NULL_VAL))
  {
    eat(TokenType::NULL_VAL, "expecting null in rvalue");
  }
  else if(match(TokenType::NEW))
  {
    new_rvalue();
  }
  else
  {
    eat(TokenType::ID, "expecting id in rvalue");
    if(match(TokenType::LPAREN))
    {
      call_expr();
    }
    else
    {
      var_rvalue();
    }
  }
}

void SimpleParser::new_rvalue()
{
  //eat the new value
  eat(TokenType::NEW, "new keyword expected");
  //if we find an id
  if(match(TokenType::ID))
  {
    eat(TokenType::ID, "expecting id in new_rvalue");
    if(match(TokenType::LBRACKET))
    {
      eat(TokenType::LBRACKET, "expecting l bracket in new rvalue");
      expr();
      if(lexer.peek() == '[') {
        eat(TokenType::RBRACKET, "expecting rbravket in new r value");
        eat(TokenType::LBRACKET, "expecting l bracket in 2d new rvalue");
        expr();
        eat(TokenType::RBRACKET, "expecting rbracket in 2d new rval");
      }
      else {
        eat(TokenType::RBRACKET, "expecting r bracket in new rvalue");
      }
    }
  }
  else //if there is a base type
  {
    if(base_type())
    {
      advance();
      eat(TokenType::LBRACKET, "expecting l bracket in btype new rvalue");
      expr();

      if(lexer.peek() == '[') {
        eat(TokenType::RBRACKET, "expecting rbravket in new r value");
        eat(TokenType::LBRACKET, "expecting l bracket in 2d new rvalue");
        expr();
        eat(TokenType::RBRACKET, "expecting rbracket in 2d new rval");
      }
      else {
        eat(TokenType::RBRACKET, "expecting r bracket in new rvalue");
      }
    }
    else{
      error("base type expected in new rvalue");
    }
  }
}

bool SimpleParser::base_rvalue()
{
  return match({TokenType::INT_VAL, TokenType::DOUBLE_VAL, TokenType::STRING_VAL, TokenType::BOOL_VAL, TokenType::CHAR_VAL});
}

void SimpleParser::var_rvalue()
{
  while(match(TokenType::DOT) || match(TokenType::LBRACKET))
  {
    if(match(TokenType::DOT))
    {
      eat(TokenType::DOT, "expecting dot in var rvalue");
      eat(TokenType::ID, "expecting id in var rvalue");
    }
    if(match(TokenType::LBRACKET))
    {
      eat(TokenType::LBRACKET, "expecting l bracket in var_value");
      expr();
     
       if(lexer.peek() == '[') {
        eat(TokenType::RBRACKET, "expecting rbravket in new r value");
        eat(TokenType::LBRACKET, "expecting l bracket in 2d new rvalue");
        expr();
        eat(TokenType::RBRACKET, "expecting rbracket in 2d new rval");
      }
      else {
        eat(TokenType::RBRACKET, "expecting r bracket in new rvalue");
      }
    }
  }
}