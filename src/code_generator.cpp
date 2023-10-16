//----------------------------------------------------------------------
// FILE: code_generator.cpp
// DATE: CPSC 326, Spring 2023
// AUTH: Nisa Meshal
// DESC: Code generator file for vm
//----------------------------------------------------------------------

#include <iostream>             // for debugging
#include "code_generator.h"

using namespace std;


// helper function to replace all occurrences of old string with new
void replace_all(string& s, const string& old_str, const string& new_str)
{
  while (s.find(old_str) != string::npos)
    s.replace(s.find(old_str), old_str.size(), new_str);
}


CodeGenerator::CodeGenerator(VM& vm)
  : vm(vm)
{
}


void CodeGenerator::visit(Program& p)
{
  for (auto& struct_def : p.struct_defs)
    struct_def.accept(*this);
  for (auto& fun_def : p.fun_defs)
    fun_def.accept(*this);
}


void CodeGenerator::visit(FunDef& f)
{
  //make a new frame for the function and set it to the current frame
  VMFrameInfo new_frame {f.fun_name.lexeme(), int(f.params.size())}; 
  curr_frame = new_frame; 
  
  //new var table for function
  var_table.push_environment(); 
  
  //iterate through params for frame
  for(auto param : f.params) {
    var_table.add(param.var_name.lexeme()); 
    //we can grab from var table to get the oid/address to store
    curr_frame.instructions.push_back(VMInstr::STORE(var_table.get(param.var_name.lexeme()))); 
  }

  //funciton body
  for(auto stmt : f.stmts) {
    stmt->accept(*this);
  }

  //if there is no return type, then add a return. 
  if(f.return_type.type_name == "void") {
    curr_frame.instructions.push_back(VMInstr::PUSH(nullptr)); 
    curr_frame.instructions.push_back(VMInstr::RET()); 
  }

  var_table.pop_environment(); 

  vm.add(curr_frame); //add the current frame info to the vm for execution
}


void CodeGenerator::visit(StructDef& s)
{
  //struct name only needs to be added, information is already generated in semantic checker
  struct_defs[s.struct_name.lexeme()] = s; 
}


void CodeGenerator::visit(ReturnStmt& s)
{
  s.expr.accept(*this); 
  curr_frame.instructions.push_back(VMInstr::RET()); 
}

//refer to class notes for explanation
void CodeGenerator::visit(WhileStmt& s)
{
  int start = curr_frame.instructions.size(); 
  s.condition.accept(*this); 
  curr_frame.instructions.push_back(VMInstr::JMPF(-1));
  int jmpf = curr_frame.instructions.size() - 1;  
  var_table.push_environment(); 
  for(auto stmt : s.stmts) {
    stmt->accept(*this);
  }
  var_table.pop_environment(); 
  curr_frame.instructions.push_back(VMInstr::JMP(start)); 
  curr_frame.instructions.push_back(VMInstr::NOP()); 
  curr_frame.instructions[jmpf] = VMInstr::JMPF(curr_frame.instructions.size() - 1); 
}

//refer to class notes for explanation
void CodeGenerator::visit(ForStmt& s)
{
  var_table.push_environment(); 
  s.var_decl.accept(*this); 
  
  int start = curr_frame.instructions.size();
  s.condition.accept(*this);
  curr_frame.instructions.push_back(VMInstr::JMPF(-1)); 
  int jmpf = curr_frame.instructions.size() -1; 

  var_table.push_environment();
  for(auto stmt : s.stmts) {
    stmt->accept(*this);
  }
  var_table.pop_environment();

  s.assign_stmt.accept(*this);
  var_table.pop_environment(); 

  curr_frame.instructions.push_back(VMInstr::JMP(start)); 
  curr_frame.instructions.push_back(VMInstr::NOP()); 
  curr_frame.instructions[jmpf] = VMInstr::JMPF(curr_frame.instructions.size() - 1); 
}


void CodeGenerator::visit(IfStmt& s)
{
  //use this to set jumpf indexes. 
  int jmpf_last = 0; //jmpf will be adjusted as the statements grow
  vector<int> jmp_indexes; //stores all the indexes of the jmp instructions, because all jmps will go to the nop at the end

  //inital if part
  s.if_part.condition.accept(*this); //generate the condition
  curr_frame.instructions.push_back(VMInstr::JMPF(-1)); //condition is false
  jmpf_last = curr_frame.instructions.size() - 1; 
  //initial if statement body, condition is true
  var_table.push_environment(); //stmt environment
  for (auto stmt : s.if_part.stmts) {
    stmt->accept(*this); 
  }
  var_table.pop_environment(); 

  //add to jmp indexes
  curr_frame.instructions.push_back(VMInstr::JMP(-1)); //jump to nop
  jmp_indexes.push_back(curr_frame.instructions.size() - 1); 

  //else if statements
  for(auto elifs : s.else_ifs) {
    //set the previous jump false to the condition that will be generated
    curr_frame.instructions[jmpf_last] = VMInstr::JMPF(curr_frame.instructions.size()); 

    //condition of the statement
    elifs.condition.accept(*this); 
    curr_frame.instructions.push_back(VMInstr::JMPF(-1)); 
    jmpf_last = curr_frame.instructions.size() - 1; //update the last jmpf index to this one (set later)

    //statement body
    var_table.push_environment(); 
    for (auto stmt : elifs.stmts) {
      stmt->accept(*this); 
    }
    var_table.pop_environment(); 

    //add to jmp indexes
    curr_frame.instructions.push_back(VMInstr::JMP(-1)); 
    jmp_indexes.push_back(curr_frame.instructions.size() - 1); 
  }

  //go through else statemets, don't need jmp because it will automatically fall to nop
  if(!s.else_stmts.empty()) {
    curr_frame.instructions[jmpf_last] = VMInstr::JMPF(curr_frame.instructions.size()); 

    var_table.push_environment(); 
    for(auto stmt : s.else_stmts) {
      stmt->accept(*this); 
    }
    var_table.pop_environment(); 
  }

  curr_frame.instructions.push_back(VMInstr::NOP()); 

  //set the jmpf for the initial if when there are no other conditions to check
  if(s.else_ifs.empty() && s.else_stmts.empty())
    curr_frame.instructions[jmpf_last] = VMInstr::JMPF(curr_frame.instructions.size() - 1); 

  //set all the jmp indexes to nop
  for(auto jmp : jmp_indexes) {
    curr_frame.instructions[jmp] = VMInstr::JMP(curr_frame.instructions.size() - 1); 
  }
}


void CodeGenerator::visit(VarDeclStmt& s)
{
  //add the new var name to symbol table
  var_table.add(s.var_def.var_name.lexeme()); 

  s.expr.accept(*this); 

  curr_frame.instructions.push_back(VMInstr::STORE(var_table.get(s.var_def.var_name.lexeme()))); //store expr into index
}


void CodeGenerator::visit(AssignStmt& s)
{
  //load initial value
  int index = var_table.get(s.lvalue[0].var_name.lexeme()); 

  curr_frame.instructions.push_back(VMInstr::LOAD(index)); //also covers 2 value path expressions x.y

  if(s.lvalue[0].array_expr.has_value()) //if array, evaluate the index that you need to push
    s.lvalue[0].array_expr->accept(*this); 

  //changes made to acommodate any 2d array expressions
  if(s.lvalue[0].array_expr_2D.has_value())
    s.lvalue[0].array_expr_2D->accept(*this); 
  
  //if the path does not exceed one, store into it or set the value at the index if it's an array
  if(s.lvalue.size() == 1) {
    if(s.lvalue[0].array_expr.has_value()) {

      s.expr.accept(*this); 

      if(s.lvalue[0].array_expr_2D.has_value())
        curr_frame.instructions.push_back(VMInstr::SETI2D()); 
      else
        curr_frame.instructions.push_back(VMInstr::SETI()); 
    }
    else {
      s.expr.accept(*this); 
      curr_frame.instructions.push_back(VMInstr::STORE(index)); 
    }
  } 
  else  { //path is greater than one
      if(s.lvalue[0].array_expr.has_value())
        curr_frame.instructions.push_back(VMInstr::GETI()); 

    if(s.lvalue[0].array_expr_2D.has_value())
      curr_frame.instructions.push_back(VMInstr::GETI2D());

    //itreate through to the second from last variable
    for(int i = 1; i < s.lvalue.size() - 1; i++) {
      curr_frame.instructions.push_back(VMInstr::GETF(s.lvalue[i].var_name.lexeme())); 
      //if the var is an array, then push(index) and geti()
      if(s.lvalue[i].array_expr.has_value()) {
        s.lvalue[i].array_expr->accept(*this); 

        if(s.lvalue[i].array_expr_2D.has_value())
          s.lvalue[i].array_expr_2D->accept(*this); 

        if(s.lvalue[i].array_expr_2D.has_value())
          curr_frame.instructions.push_back(VMInstr::GETI2D()); 
        else 
          curr_frame.instructions.push_back(VMInstr::GETI()); 
      }
    }

    //end of path,seti if array, setf if field
    if(s.lvalue.back().array_expr.has_value()) {
      curr_frame.instructions.push_back(VMInstr::GETF(s.lvalue.back().var_name.lexeme())); 
      if(s.lvalue.back().array_expr_2D.has_value()) 
        s.lvalue.back().array_expr_2D->accept(*this); 
    
      s.lvalue.back().array_expr->accept(*this); 
      s.expr.accept(*this); 

      if(s.lvalue.back().array_expr_2D.has_value())
        curr_frame.instructions.push_back(VMInstr::SETI2D()); 
      else
        curr_frame.instructions.push_back(VMInstr::SETI()); 
    }
    else {
      s.expr.accept(*this); 
      curr_frame.instructions.push_back(VMInstr::SETF(s.lvalue.back().var_name.lexeme())); 
    }
  }
}


void CodeGenerator::visit(CallExpr& e)
{
  //push all arguments onto stack
  for(auto ex : e.args) {
    ex.accept(*this); 
  }

  string fun_name = e.fun_name.lexeme(); 

  //check for built ins, if not then it's just a call instruction
  if(fun_name == "concat")
    curr_frame.instructions.push_back(VMInstr::CONCAT());  
  else if (fun_name == "input")
    curr_frame.instructions.push_back(VMInstr::READ()); 
  else if (fun_name == "get")
    curr_frame.instructions.push_back(VMInstr::GETC()); 
  else if (fun_name == "print") 
    curr_frame.instructions.push_back(VMInstr::WRITE()); 
  else if (fun_name == "to_string")
    curr_frame.instructions.push_back(VMInstr::TOSTR());
  else if (fun_name == "to_int")
    curr_frame.instructions.push_back(VMInstr::TOINT()); 
  else if (fun_name == "to_double")
    curr_frame.instructions.push_back(VMInstr::TODBL()); 
  else if (fun_name == "length")
    curr_frame.instructions.push_back(VMInstr::SLEN()); 
  else if (fun_name == "length@array")
    curr_frame.instructions.push_back(VMInstr::ALEN()); 
  else 
    curr_frame.instructions.push_back(VMInstr::CALL(fun_name)); 
}


void CodeGenerator::visit(Expr& e)
{  
  e.first->accept(*this); 

  //check if rest has value
  if(e.rest != nullptr) {
    e.rest->accept(*this); 

    //check op types and push
    if(e.op->type() == TokenType::PLUS)
      curr_frame.instructions.push_back(VMInstr::ADD()); 
    else if (e.op->type() == TokenType::MINUS)
      curr_frame.instructions.push_back(VMInstr::SUB());
    else if (e.op->type() == TokenType::TIMES)
      curr_frame.instructions.push_back(VMInstr::MUL()); 
    else if (e.op->type() == TokenType::DIVIDE)
      curr_frame.instructions.push_back(VMInstr::DIV()); 
    else if (e.op->type() == TokenType::EQUAL)
      curr_frame.instructions.push_back(VMInstr::CMPEQ()); 
    else if (e.op->type() == TokenType::NOT_EQUAL)
      curr_frame.instructions.push_back(VMInstr::CMPNE()); 
    else if (e.op->type() == TokenType::LESS)
      curr_frame.instructions.push_back(VMInstr::CMPLT()); 
    else if (e.op->type() == TokenType::GREATER)
      curr_frame.instructions.push_back(VMInstr::CMPGT()); 
    else if (e.op->type() == TokenType::LESS_EQ)
      curr_frame.instructions.push_back(VMInstr::CMPLE()); 
    else if (e.op->type() == TokenType::GREATER_EQ)
      curr_frame.instructions.push_back(VMInstr::CMPGE()); 
    else if (e.op->type() == TokenType::AND)
      curr_frame.instructions.push_back(VMInstr::AND());
    else if (e.op->type() == TokenType::OR)
      curr_frame.instructions.push_back(VMInstr::OR()); 
  } 

  //negate the expression at the end, once everything is loaded in 
  if(e.negated)
    curr_frame.instructions.push_back(VMInstr::NOT()); 
}


void CodeGenerator::visit(SimpleTerm& t)
{
  t.rvalue->accept(*this); 
}
 

void CodeGenerator::visit(ComplexTerm& t)
{
  t.expr.accept(*this); 
}

//refer to lecture notes for expalanations
void CodeGenerator::visit(SimpleRValue& v)
{
  if (v.value.type() == TokenType::INT_VAL) {
    int val = stoi(v.value.lexeme()); 
    curr_frame.instructions.push_back(VMInstr::PUSH(val)); 
  }
  else if (v.value.type() == TokenType::DOUBLE_VAL) {
    double val = stod(v.value.lexeme());
    curr_frame.instructions.push_back(VMInstr::PUSH(val)); 
  }
  else if (v.value.type() == TokenType::NULL_VAL) {
    curr_frame.instructions.push_back(VMInstr::PUSH(nullptr));
  }
  else if (v.value.type() == TokenType::BOOL_VAL) {
    if(v.value.lexeme() == "true")
      curr_frame.instructions.push_back(VMInstr::PUSH(true)); 
    else
      curr_frame.instructions.push_back(VMInstr::PUSH(false)); 
  }
  else {
    string s = v.value.lexeme(); 
    replace_all(s, "\\n", "\n"); 
    replace_all(s, "\\t", "\t");
    replace_all(s, "\\'", "\'");
    curr_frame.instructions.push_back(VMInstr::PUSH(s));  
  }
}


void CodeGenerator::visit(NewRValue& v)
{
  //check for new array expression, if not, it's a new struct
  //if 1D array, push column and null
  //if 2D array, push in column, row, and null
  if(v.array_expr.has_value()) {  
    if(v.array_expr_2D.has_value()) { //for 2D arrays the columns counts are stored in the 2nd array expression
      v.array_expr_2D->accept(*this); 
    }

    v.array_expr->accept(*this); //if 2D this stores the row counts, if 1D this stores columns

    curr_frame.instructions.push_back(VMInstr::PUSH(nullptr)); //initial set to null

    if(v.array_expr_2D.has_value()) 
      curr_frame.instructions.push_back(VMInstr::ALLOCA2D()); 
    else
      curr_frame.instructions.push_back(VMInstr::ALLOCA()); 
  }
  else { //not an array so has to be a struct
    curr_frame.instructions.push_back(VMInstr::ALLOCS()); 
    //loop to add and initialize fields
    for(auto field : struct_defs[v.type.lexeme()].fields) {
      curr_frame.instructions.push_back(VMInstr::DUP()); 
      curr_frame.instructions.push_back(VMInstr::ADDF(field.var_name.lexeme())); 
      curr_frame.instructions.push_back(VMInstr::DUP()); 
      curr_frame.instructions.push_back(VMInstr::PUSH(nullptr)); 
      curr_frame.instructions.push_back(VMInstr::SETF(field.var_name.lexeme())); 
    }
  }
}


void CodeGenerator::visit(VarRValue& v)
{
  //initial value load
  int index = var_table.get(v.path[0].var_name.lexeme());
  curr_frame.instructions.push_back(VMInstr::LOAD(index));
  if(v.path[0].array_expr.has_value()) {

    v.path[0].array_expr->accept(*this);

    if(v.path[0].array_expr_2D.has_value())
      v.path[0].array_expr_2D->accept(*this); 

    if(v.path[0].array_expr_2D.has_value())
      curr_frame.instructions.push_back(VMInstr::GETI2D()); 
    else
      curr_frame.instructions.push_back(VMInstr::GETI()); 
  }

  //for path expressions
  for(int i = 1; i < v.path.size(); i++) {
    curr_frame.instructions.push_back(VMInstr::GETF(v.path[i].var_name.lexeme()));
    if(v.path[i].array_expr.has_value()) {

      v.path[i].array_expr->accept(*this); 

      if(v.path[i].array_expr_2D.has_value())
        v.path[i].array_expr_2D->accept(*this); 

      if(v.path[i].array_expr_2D.has_value())
        curr_frame.instructions.push_back(VMInstr::GETI2D());
      else
        curr_frame.instructions.push_back(VMInstr::GETI()); 
    }
  }
}
    