//----------------------------------------------------------------------
// FILE: semantic_checker.cpp
// DATE: CPSC 326, Spring 2023
// AUTH: Nisa Meshal
// DESC: MyPL semantic checker
//----------------------------------------------------------------------

#include <unordered_set>
#include <iostream> 
#include "mypl_exception.h"
#include "semantic_checker.h"


using namespace std;

// hash table of names of the base data types and built-in functions
const unordered_set<string> BASE_TYPES {"int", "double", "char", "string", "bool"};
const unordered_set<string> BUILT_INS {"print", "input", "to_string",  "to_int",
  "to_double", "length", "length@array","get", "concat"};


// helper functions

optional<VarDef> SemanticChecker::get_field(const StructDef& struct_def,
                                            const string& field_name)
{
  for (const VarDef& var_def : struct_def.fields)
    if (var_def.var_name.lexeme() == field_name)
      return var_def;
  return nullopt;
}


void SemanticChecker::error(const string& msg, const Token& token)
{
  string s = msg;
  s += " near line " + to_string(token.line()) + ", ";
  s += "column " + to_string(token.column());
  throw MyPLException::StaticError(s);
}


void SemanticChecker::error(const string& msg)
{
  throw MyPLException::StaticError(msg);
}


// visitor functions


void SemanticChecker::visit(Program& p)
{
  // record each struct def
  for (StructDef& d : p.struct_defs) {
    string name = d.struct_name.lexeme();
    if (struct_defs.contains(name))
      error("multiple definitions of '" + name + "'", d.struct_name);
    struct_defs[name] = d;
  }

  // record each function def (need a main function)
  bool found_main = false;
  for (FunDef& f : p.fun_defs) {
    string name = f.fun_name.lexeme();
    if (BUILT_INS.contains(name))
      error("redefining built-in function '" + name + "'", f.fun_name);
    if (fun_defs.contains(name))
      error("multiple definitions of '" + name + "'", f.fun_name);
    if (name == "main") {
      if (f.return_type.type_name != "void")
        error("main function must have void type", f.fun_name);
      if (f.params.size() != 0)
        error("main function cannot have parameters", f.params[0].var_name);
      found_main = true;
    }
    fun_defs[name] = f;
  }

  if (!found_main)
    error("program missing main function");
  // check each struct
  for (StructDef& d : p.struct_defs)
    d.accept(*this);
  // check each function
  for (FunDef& d : p.fun_defs)
    d.accept(*this);
}


void SemanticChecker::visit(SimpleRValue& v)
{
  if (v.value.type() == TokenType::INT_VAL)
    curr_type = DataType {false, "int"};
  else if (v.value.type() == TokenType::DOUBLE_VAL)
    curr_type = DataType {false,  "double"};    
  else if (v.value.type() == TokenType::CHAR_VAL)
    curr_type = DataType {false, "char"};    
  else if (v.value.type() == TokenType::STRING_VAL)
    curr_type = DataType {false, "string"};    
  else if (v.value.type() == TokenType::BOOL_VAL)
    curr_type = DataType {false, "bool"};    
  else if (v.value.type() == TokenType::NULL_VAL)
    curr_type = DataType {false, "void"};    
}


// TODO: Implement the rest of the visitor functions (stubbed out below)

void SemanticChecker::visit(FunDef& f)
{
  //first function environment
  symbol_table.push_environment(); 

  //check if the return type is a base type, if not then check that it's in struct defs
  if(BASE_TYPES.contains(f.return_type.type_name) || f.return_type.type_name == "void") {
    curr_type = f.return_type;
    symbol_table.add("return", curr_type); 
  }
  else {
    if(!struct_defs.contains(f.return_type.type_name)) {
      error("struct return type is undefined"); 
    }
    else {
      curr_type = f.return_type;
      symbol_table.add("return", curr_type); 
    }
  }

  if(!f.params.empty()) {
    for(auto v : f.params) {
      curr_type = v.data_type;
      if(symbol_table.name_exists_in_curr_env(v.var_name.lexeme())) {
        error("paramater name already exists"); 
      }
      else if (!BASE_TYPES.contains(curr_type.type_name) &&!struct_defs.contains(v.data_type.type_name)) {
        error("reference to undefined struct in function params"); 
      }
      else {
        symbol_table.add(v.var_name.lexeme(), curr_type);
      }
    }
  }

  for(auto s : f.stmts) {
    s->accept(*this);
  }

  symbol_table.pop_environment();
}


void SemanticChecker::visit(StructDef& s)
{
  symbol_table.push_environment();

  if(struct_defs.contains(s.struct_name.lexeme())) {
    DataType dt = {false, "struct"}; 
    symbol_table.add(s.struct_name.lexeme(), dt);  

    for(VarDef v : s.fields) {
      //check if the field is a base type
      if(BASE_TYPES.contains(v.data_type.type_name)) {
        if(symbol_table.name_exists_in_curr_env(v.var_name.lexeme()))
          error("varible name already exists in struct fields"); 

        curr_type = v.data_type;
        symbol_table.add(v.var_name.lexeme(), curr_type);
      } 
      else { //if not a base type, then it must be a stuct
        if(!struct_defs.contains(v.data_type.type_name)) {
          error("reference to undefined struct"); 
        } 
        else {
          curr_type = v.data_type; 
          symbol_table.add(v.var_name.lexeme(), curr_type); 
        } 
      }
    } 
  }

  symbol_table.pop_environment();
}


void SemanticChecker::visit(ReturnStmt& s)
{
  s.expr.accept(*this);

  DataType d = symbol_table.get("return").value(); 

  if(curr_type.type_name != d.type_name &&  curr_type.type_name != "void") {
    error("return statement does not match function return in " + curr_type.type_name + " " + d.type_name); 
  }
}


void SemanticChecker::visit(WhileStmt& s)
{
  symbol_table.push_environment(); 
  s.condition.accept(*this); 

  if(curr_type.type_name != "bool") {
    error("while condition is not of type bool"); 
  }

  for(auto st : s.stmts) {
    st->accept(*this); 
  }

  symbol_table.pop_environment(); 
}


void SemanticChecker::visit(ForStmt& s)
{
  symbol_table.push_environment(); 

  s.var_decl.accept(*this);
  if(curr_type.type_name != "int") {
    error("loop counter is not an integer");
  }

  s.condition.accept(*this);
  if(curr_type.type_name != "bool") {
    error("loop condition is not a bool"); 
  }

  s.assign_stmt.accept(*this); 
  if(curr_type.type_name != "int") {
    error("loop increment is not an integer"); 
  }
   
  for(auto st : s.stmts) {
    st->accept(*this); 
  }
  
  symbol_table.pop_environment(); 
}


void SemanticChecker::visit(IfStmt& s)
{
  //if part in its own environment
  symbol_table.push_environment();

  s.if_part.condition.accept(*this);
  if(curr_type.type_name != "bool" || curr_type.is_array == true) {
    error("if condition not a bool"); 
  }

  for(auto st : s.if_part.stmts) {
    st->accept(*this); 
  }

  symbol_table.pop_environment(); //???? do i pop this here?

  //else ifs
  if(!s.else_ifs.empty()) {
    for(auto elif : s.else_ifs) {
      //new environment for every else if block
      symbol_table.push_environment(); 

      elif.condition.accept(*this); 
      if(curr_type.type_name != "bool") {
        error("else if condition is not a bool"); 
      }

      for(auto st : elif.stmts) {
        st->accept(*this); 
      }

      symbol_table.pop_environment();
    }
  }

  //else statements
  if(!s.else_stmts.empty()) {
    symbol_table.push_environment();
    for(auto st : s.else_stmts) {
      st->accept(*this);
    }
    symbol_table.pop_environment();
  }
}


void SemanticChecker::visit(VarDeclStmt& s)
{
  //grab the data type of the var decl
  DataType d = s.var_def.data_type; 
  //if the name of the variable dne, then add it with its data type
  if(!symbol_table.name_exists_in_curr_env(s.var_def.var_name.lexeme())) {
    symbol_table.add(s.var_def.var_name.lexeme(), d);
  } else {
    error("name is already previously declared");
  }

  //check the expression, sets currtype
  s.expr.accept(*this);

  if(d.type_name != curr_type.type_name && curr_type.type_name != "void")
    error("mismatched types in variable declaration");
}


void SemanticChecker::visit(AssignStmt& s)
{
  //check that the first value is in symbol table
  if(!symbol_table.name_exists(s.lvalue[0].var_name.lexeme()))
    error("reference to undefined variable: " + s.lvalue[0].var_name.lexeme()); 

  //if the first value is an array, then check the array expression 
  if(s.lvalue[0].array_expr.has_value()) {
    //check the array expression
    DataType array_type = symbol_table.get(s.lvalue[0].var_name.lexeme()).value(); 

    s.lvalue[0].array_expr->accept(*this); 
    if(curr_type.type_name != "int")
      error("array expression does not contain int"); 
    
    if(s.lvalue[0].array_expr_2D.has_value()){
      s.lvalue[0].array_expr_2D->accept(*this);
      if(curr_type.type_name != "int")
        error("array expression does not contain int");
    }
    
    curr_type = array_type; 
  }
  else { //single variable case. 
    curr_type = symbol_table.get(s.lvalue[0].var_name.lexeme()).value();
  }

  //if the path is greater than one, the first value type must be in struct def
  if(s.lvalue.size() > 1) {
    //get the type of the variable
    DataType var_type = symbol_table.get(s.lvalue[0].var_name.lexeme()).value(); 

    //graq the first struct def
    StructDef sd = struct_defs[var_type.type_name]; 

    //iterate through te rest of the path
    for(int i = 1; i < s.lvalue.size(); i++) {
      //check that the field is in the previous struct def
      if(!get_field(sd, s.lvalue[i].var_name.lexeme()))
        error("field does not exist in struct");

      //set the current type
      curr_type = get_field(sd, s.lvalue[i].var_name.lexeme()).value().data_type;

      //set struct def again. 
      sd = struct_defs[curr_type.type_name]; 
    }
  }

  DataType assign_lval = curr_type; 

  //rhs of assign statement 
  s.expr.accept(*this); 

  if(assign_lval.type_name != curr_type.type_name && curr_type.type_name != "void") {
    error("mismatched type in assign statement"); 
  }
}


void SemanticChecker::visit(CallExpr& e)
{ 
  //check that the function is built in or not
  if(BUILT_INS.contains(e.fun_name.lexeme())) {
    //check for concat
    if(e.fun_name.lexeme() == "concat") { //since concat is the only one that takes more than one argument
      if(e.args.size() != 2)
        error("need two args for concat");

      e.args[0].accept(*this);
      if(curr_type.type_name != "string")
        error("need string in concat");

      e.args[1].accept(*this);
      if(curr_type.type_name != "string")
        error("need string in concat"); 

      curr_type = DataType {false, "string"}; 
    } 
    //input function
    else if(e.fun_name.lexeme() == "input") {
      if(e.args.size() != 0) {
        error("too many arguments in input funtion"); 
      }

      curr_type = DataType {false, "string"}; 
    }
    //get function
    else if (e.fun_name.lexeme() == "get") {
      if(e.args.size() != 2)
        error("get function requires two arguments"); 

      e.args[0].accept(*this);
      if(curr_type.type_name != "int")
        error("first get argument must be an int"); 

      e.args[1].accept(*this);
      if(curr_type.type_name != "string")
        error("need string for second argument"); 

      curr_type = DataType {false, "char"}; 
    }
    //other built ins that only take one argument
    else {
      if(e.args.size() > 1) { //these functions must have only one argument. 
        error("too many function arguments for " + e.fun_name.lexeme()); 
      }

      //evaluate the expression and 
      e.args[0].accept(*this); 

      //print built in function
      if(e.fun_name.lexeme() == "print") {
        //print can only be base type and can't be an array
        if(!BASE_TYPES.contains(curr_type.type_name)) {  //i took out array check 
          error("a base type must be used in the built in print function " + curr_type.type_name); 
        }

        curr_type = DataType {false, "void"}; 
      }
      //to_string built on me function
      else if(e.fun_name.lexeme() == "to_string") {
        unordered_set<string> TO_STRING_TYPES = {"int", "double", "char"};
        
        if(!TO_STRING_TYPES.contains(curr_type.type_name) || curr_type.is_array) {
          error("invalid to_string arguments"); 
        }

        curr_type = DataType {false, "string"}; 
      }
      //to_int built in funciton
      else if(e.fun_name.lexeme() == "to_int") {
        if(curr_type.type_name != "string" && curr_type.type_name != "double") {
          error("invalid argument for to_int function");
        }

        curr_type = DataType {false, "int"}; 
      }
      else if(e.fun_name.lexeme() == "to_double") {
        if(curr_type.type_name != "int" && curr_type.type_name != "string") {
          error("invalid argument for to_double function");
        }

        curr_type = DataType {false, "double"}; 
      }
      else if(e.fun_name.lexeme() == "length") {
        //length function must be passed a string or an array
        if(curr_type.type_name != "string" && !curr_type.is_array)
          error("invalid argument for length function"); 

        //if you find an array, then must chnage the function name for proper call in code gen
        if(curr_type.is_array) {
          curr_type = DataType {true, "int"};
          e.fun_name = Token(e.fun_name.type(), "length@array", e.fun_name.line(), e.fun_name.column());
        }
        else
          curr_type = DataType {false, "int"};
      }
    }

  }
  else if(!fun_defs.contains(e.fun_name.lexeme())) { //check if function exists in current environment
    error("function does not exist in this environment");
  }
  else { //function is in fun_defs, so check for arguments. 
    //check if function call has proper number of arguments
    if(e.args.size() != fun_defs[e.fun_name.lexeme()].params.size()) {
      error("incorrect number of arguments passed in function call"); 
    }
    else { //check argument types match
      FunDef f = fun_defs[e.fun_name.lexeme()];
      
      for(int i = 0; i < f.params.size(); i++) {
        e.args[i].accept(*this); //set curr_token type
        if(curr_type.type_name != f.params[i].data_type.type_name && curr_type.type_name != "void") { //check taht paramater types match
          error("parameter type mismatch in function call"); 
        }
      }
      
      curr_type = f.return_type; 
    }
  }
}


void SemanticChecker::visit(Expr& e)
{
  e.first->accept(*this);
  DataType lhs = curr_type;

  if(e.op.has_value()) {
    
    e.rest->accept(*this); 
    DataType rhs = curr_type;
    Token op = e.op.value();

    //arithmetic ops
    if (op.type() == TokenType::PLUS || op.type() == TokenType::MINUS || op.type() == TokenType::TIMES || op.type() == TokenType::DIVIDE) {
      if(lhs.type_name != "int" && lhs.type_name != "double") {
        error("type incompatible with arithmatic expresssion");
      }

      if(rhs.type_name != "int" && rhs.type_name != "double") {
        error("type incompatible with arithmetic expression"); 
      }

      if(lhs.type_name == "int" && rhs.type_name == "int") {
        curr_type = DataType {false, "int"}; 
      }
      else {
        curr_type = DataType {false, "double"}; 
      }
    }
    //equality ops
    else if (op.type() == TokenType::EQUAL || op.type() == TokenType::NOT_EQUAL) { 
      if(!(lhs.type_name == rhs.type_name || lhs.type_name == "void" || rhs.type_name == "void")) {
        error("types incompatible in boolean expression");
      }
      curr_type = DataType {false, "bool"}; 
    }
    //comparators 
    else if (op.type() == TokenType::LESS || op.type() == TokenType::GREATER || op.type() == TokenType::LESS_EQ || op.type() == TokenType::GREATER_EQ) {
      if(lhs.type_name != "int" && lhs.type_name != "double" && lhs.type_name != "string" && lhs.type_name != "char") {
        error("lhs type incompatible in comparison expression"); 
      }

      if(rhs.type_name != "int" && rhs.type_name != "double" && rhs.type_name != "string" && rhs.type_name != "char") {
        error("rhs type incompatible in comparison expression"); 
      }
      curr_type = DataType {false, "bool"}; 
    }
    else if (op.type() == TokenType::AND || op.type() == TokenType::OR) {
      if(!(lhs.type_name == "bool" && rhs.type_name == "bool")) {
        error("values incompatible with and/or expression"); 
      }
      curr_type = DataType {false, "bool"}; 
    }
  }

  if(e.negated == true && curr_type.type_name != "bool") {
    error("type mismatch in negated statement"); 
  }

}


void SemanticChecker::visit(SimpleTerm& t)
{
  t.rvalue->accept(*this); 
} 


void SemanticChecker::visit(ComplexTerm& t)
{
  t.expr.accept(*this); 
}


void SemanticChecker::visit(NewRValue& v)
{
  if(v.array_expr.has_value()) {
    v.array_expr->accept(*this);
    if(curr_type.type_name != "int")
      error("array expr is not an int"); 
    
    if(v.array_expr_2D.has_value()) {
      v.array_expr_2D->accept(*this);
      if(curr_type.type_name != "int")
        error("array expr is not an int");
    }

    curr_type = DataType {true, v.type.lexeme()}; 
  }
  else {
    curr_type = DataType {false, v.type.lexeme()};
  }
}


void SemanticChecker::visit(VarRValue& v)
{
  //check that the first value is in symbol table
  if(!symbol_table.name_exists(v.path[0].var_name.lexeme())) 
    error("reference to undefined variable var_rval " + v.path[0].var_name.lexeme() + " in "); 

  //if the first value is an array, then check the array expression 
  if(v.path[0].array_expr.has_value() && v.path[0].array_expr_2D.has_value()) {
    //check the array expression
    DataType array_type = symbol_table.get(v.path[0].var_name.lexeme()).value(); 

    v.path[0].array_expr->accept(*this); 

    if(curr_type.type_name != "int")
      error("array expression does not contain int"); 

    if(v.path[0].array_expr_2D.has_value()) {
      v.path[0].array_expr_2D->accept(*this);

      if(curr_type.type_name != "int")
        error("array expression does not contain int");
    }
    
    curr_type = array_type; 
  }
  else { //single variable case. 
    curr_type = symbol_table.get(v.path[0].var_name.lexeme()).value();
  }

  //if the path is greater than one, the first value type must be in struct def
  if(v.path.size() > 1) {
    //get the type of the variable
    DataType var_type = symbol_table.get(v.path[0].var_name.lexeme()).value(); 

    //graq the first struct def
    StructDef sd = struct_defs[var_type.type_name]; 

    //iterate through te rest of the path
    for(int i = 1; i < v.path.size(); i++) {
      //check that the field is in the previous struct def
      if(!get_field(sd, v.path[i].var_name.lexeme()))
        error("field does not exist in struct");

      //set the current type
      curr_type = get_field(sd, v.path[i].var_name.lexeme()).value().data_type;

      //set struct def again. 
      sd = struct_defs[curr_type.type_name]; 
    }
  }
}    

