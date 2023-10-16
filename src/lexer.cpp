//----------------------------------------------------------------------
// FILE: lexer.cpp
// DATE: CPSC 326, Spring 2023
// NAME: Nisa Meshal
// DESC: Lexer cpp file for the lexer functionality. 
//----------------------------------------------------------------------

#include "lexer.h"
#include <iostream>

using namespace std;


Lexer::Lexer(istream& input_stream)
  : input {input_stream}, column {0}, line {1}
{}


char Lexer::read()
{
  ++column;
  return input.get();
}


char Lexer::peek()
{
  return input.peek();
}


void Lexer::error(const string& msg, int line, int column) const
{
  throw MyPLException::LexerError(msg + " at line " + to_string(line) +
                                  ", column " + to_string(column));
}


Token Lexer::next_token()
{
  char currCh = read();
  int startCol = 0;
  string buildstr = "";


  //check for newline
  while(isspace(currCh) || currCh == '#') {
    if(currCh == '\n') {
      line++;
      column = 0;
      currCh = read();
    } 
    else if (currCh == '#') 
    {
      while(currCh != '\n' && currCh != EOF) 
      {
        currCh = read();
      }
    } 
    else if (currCh == EOF) 
    {
      return Token(TokenType::EOS, "EOS", line, column);
    } 
    else {
      currCh = read();
    }
  }

  //check for EOF
  if(currCh == EOF) {
    return Token(TokenType::EOS, "EOS", line, column);
  }

  //puntuation
  if(currCh == '.') {
    return Token(TokenType::DOT, ".", line, column);
  } else if (currCh == ',') {
    return Token(TokenType::COMMA, ",", line, column);
  } else if (currCh == '(') {
    return Token(TokenType::LPAREN, "(", line, column);
  } else if (currCh == ')') {
    return Token(TokenType::RPAREN, ")", line, column);
  } else if (currCh == '[') {
    return Token(TokenType::LBRACKET, "[", line, column);
  } else if (currCh == ']') {
    return Token(TokenType::RBRACKET, "]", line, column);
  } else if (currCh == ';') {
    return Token(TokenType::SEMICOLON, ";", line, column);
  } else if (currCh == '{') {
    return Token(TokenType::LBRACE, "{", line, column);
  } else if (currCh == '}') {
    return Token(TokenType::RBRACE, "}", line, column);
  }
  
  //operators and comparators
  if(currCh == '+') {
    return Token(TokenType::PLUS, "+", line, column);
  } else if (currCh == '-') {
    return Token(TokenType::MINUS, "-", line, column);
  } else if (currCh == '*') {
    return Token(TokenType::TIMES, "*", line, column);
  } else if (currCh == '/') {
    return Token(TokenType::DIVIDE, "/", line, column);
  } else if (currCh == '=') {
    //equality check
    //column must be decremented since column returns the starting index of the symbol
    if (peek() == '=') {
      startCol = column;
      currCh = read();
      return Token(TokenType::EQUAL, "==", line, startCol);
    } else {
      return Token(TokenType::ASSIGN, "=", line, column);
    }
  } else if (currCh == '<') {
    //leq check
    if (peek() == '=') {
      startCol = column;
      currCh = read();
      return Token(TokenType::LESS_EQ, "<=", line, startCol);
    } else {
      return Token(TokenType::LESS, "<", line, column); 
    }
  } else if (currCh == '>') {
    if (peek() == '=') {
      startCol = column;
      currCh = read();
      return Token(TokenType::GREATER_EQ, ">=", line, startCol);
    } else {
      return Token(TokenType::GREATER, ">", line, column);
    }
  } else if (currCh ==  '!') {
    if (peek() == '=') {
      startCol = column;
      currCh = read();
      return Token(TokenType::NOT_EQUAL, "!=", line, startCol);
    } else {
      currCh = read(); //reads next invalid
      error("expecting '!=' found '!" + string(1, currCh) + "'", line, column -1);
    }
  }

  //chars
  if(currCh == '\'') {
    char temp; 
    //empty char
    if(peek() == '\'') {
      currCh = read();
      error("empty character", line, column);
    }

    currCh = read();
    if(currCh == '\n') {
      startCol = column; 
      currCh = read(); //read the closing '
      line++;
      error("found end-of-line in character", line - 1, startCol);

    }else if(currCh == EOF) {
      error("found end-of-file in character", line, column);

    } else if(currCh == '\\') { //for tabs and newline
      currCh = read(); //reads in the second 
      temp = currCh; //stores either n or t
      currCh = read();

      if(currCh != '\'') {
        error("expecting ' found " + string(1, currCh), line, column);
      }

      if(temp == 'n') {
        return Token(TokenType::CHAR_VAL, "\\n", line, column - 3);

      } else if (temp == 't') {
        return Token(TokenType::CHAR_VAL, "\\t", line, column - 3);

      } else {
        error("char not recognized " + string(1, temp), line, column - 3);

      }
    }  else if(peek() != '\'') { //char is too long
      currCh = read(); //read the next char
      temp = currCh; //store the extra char
      startCol = column;
      
      while(currCh != '\'') { //read until char closed
        currCh = read();
      }
        
      error("expecting ' found " + string(1, temp), line, startCol);
    } else { //regular char case
      temp = currCh; 
      currCh = read(); //read close

      return Token(TokenType::CHAR_VAL, string(1, temp), line, column - 2);
    }

  }

  //strings
  if (currCh == '\"') {
    buildstr = "";
    startCol = column;

    if(peek() == EOF) {
      currCh = read();
      error("found end-of-file in string", line, column);
    }

    string buildstr = "";
    currCh = read(); //read the next value after "
    
    while(currCh != '\"') {
      if(currCh == '\n') {
        startCol = column; 
        line++;
        error("found end-of-line in string", line - 1, startCol);
      } else if (currCh == EOF) {
        error("found end-of-file in string", line, column);
      }
      buildstr.append(1, currCh);
      currCh = read();
    }
    return Token(TokenType::STRING_VAL, buildstr, line, startCol);
  }

  if(isdigit(currCh)) {
    buildstr = "";
    startCol = column;

    if(currCh == '0' && isdigit(peek())) {
      error("leading zero in number", line, column);
    }

    while(isdigit(currCh)) {
      buildstr.append(string(1, currCh));
      if(isalpha(peek()) || peek() == ';' || peek() == ')' || peek() == '}' || peek() == ']' || peek() == '+' || peek() == '/' || peek() == '-' || peek() == '*') {
        break; 
      }
      currCh = read();
    }

    if(currCh == '.') {
      buildstr.append(string(1, currCh));
    
      if(!isdigit(peek())) 
      {
        error("missing digit in '" + buildstr + "'", line, column + 1);
      } else {
        currCh = read();
        while(isdigit(currCh)) 
        {
          buildstr.append(string(1, currCh));
          if(!isdigit(peek())) 
          {
            break; 
          }
          currCh = read();
        }
      }
      return Token(TokenType::DOUBLE_VAL, buildstr, line, startCol);
    }
    return Token(TokenType::INT_VAL, buildstr, line, startCol);
  } 
  else if (isalpha(currCh)) 
  {
    buildstr = "";
    startCol = column; 

    while(!isspace(currCh) && currCh != EOF) 
    {
      buildstr.append(string(1, currCh));
      if(peek() == '=' || peek() =='(' || peek() == ')' || peek() == '<' || peek() == '>' || 
      peek() == '[' || peek() == ',' || peek() == ';' || peek() == '}' || peek() == '{' || 
      peek() == '.' || peek() == ']' || peek() == '-' || peek() == '+' || peek() == '/' || peek() == '*') 
      {
        break; 
      }
      currCh = read();
    }
  } 
  else 
  {
    error("unexpected character '" + string(1, currCh) + "'", line, column - 0);
  }
  

  if(buildstr == "int") 
  {
    return Token(TokenType::INT_TYPE, "int", line, startCol);
  } 
  else if (buildstr == "double") 
  {
    return Token(TokenType::DOUBLE_TYPE, "double", line, startCol);
  } 
  else if (buildstr == "bool") 
  {
    return Token(TokenType::BOOL_TYPE, "bool", line, startCol);
  } 
  else if (buildstr == "string") 
  {
    return Token(TokenType::STRING_TYPE, "string", line, startCol);
  } 
  else if (buildstr == "char") 
  {
    return Token(TokenType::CHAR_TYPE, "char", line, startCol);
  } 
  else if (buildstr == "void") 
  {
    return Token(TokenType::VOID_TYPE, "void", line, startCol);
  } 
  else if (buildstr == "struct") 
  {
    return Token(TokenType::STRUCT, "struct", line, startCol);
  } else if (buildstr == "array") {
    return Token(TokenType::ARRAY, "array", line, startCol);
  } else if (buildstr == "for") {
    return Token(TokenType::FOR, "for", line, startCol);
  } else if (buildstr == "while") {
    return Token(TokenType::WHILE, "while", line, startCol);
  } else if (buildstr == "if") {
    return Token(TokenType::IF, "if", line, startCol);
  } else if (buildstr == "elseif") {
    return Token(TokenType::ELSEIF, "elseif", line, startCol);
  } else if (buildstr == "else") {
    return Token(TokenType::ELSE, "else", line, startCol);
  } else if (buildstr == "and") {
    return Token(TokenType::AND, "and", line, startCol);
  } else if (buildstr == "or") {
    return Token(TokenType::OR, "or", line, startCol);
  } else if (buildstr == "not") {
    return Token(TokenType::NOT, "not", line, startCol);
  } else if (buildstr == "new") {
    return Token(TokenType::NEW, "new", line, startCol);
  } else if (buildstr == "return") {
    return Token(TokenType::RETURN, "return", line, startCol);
  } else if (buildstr == "null") {
    return Token(TokenType::NULL_VAL, "null", line, startCol);
  } else if (buildstr == "true") {
    return Token(TokenType::BOOL_VAL, "true", line, startCol);
  } else if (buildstr == "false") {
    return Token(TokenType::BOOL_VAL, "false", line, startCol);
  }

  return Token(TokenType::ID, buildstr, line, startCol);
}