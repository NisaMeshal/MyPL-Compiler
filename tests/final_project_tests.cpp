//----------------------------------------------------------------------
// FILE: final_project_tests.cpp
// DATE: CPSC 326, Spring 2023
// AUTH: Nisa Mehsal
// DESC: 2D array language extension
//----------------------------------------------------------------------

#include <iostream>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include "mypl_exception.h"
#include "simple_parser.h"
#include "lexer.h"
#include "ast_parser.h"
#include "vm.h"
#include "code_generator.h"
#include "semantic_checker.h"

using namespace std;


streambuf* stream_buffer;


void change_cout(stringstream& out)
{
  stream_buffer = cout.rdbuf();
  cout.rdbuf(out.rdbuf());
}

void restore_cout()
{
  cout.rdbuf(stream_buffer);
}

string build_string(initializer_list<string> strs)
{
  string result = "";
  for (string s : strs)
    result += s + "\n";
  return result;
}


//----------------------------------------------------------------------
// Simple Parser Tests
//----------------------------------------------------------------------
TEST(SimpleParserTests, Simple2DArray){
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
    "}"
  }));
  SimpleParser(Lexer(in)).parse(); 
}

TEST(SimpleParserTests, ArrayAssign) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
      "xs[0][0] = 1",
    "}"
  }));
  SimpleParser(Lexer(in)).parse(); 
}

TEST(SimpleParserTests, ArrayAccess) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
      "xs[0][0] = 1",
      "int x = xs[0][0]",
    "}"
  }));
  SimpleParser(Lexer(in)).parse(); 
}

//----------------------------------------------------------------------
// AST Parser Tests
//----------------------------------------------------------------------

TEST(ASTParserTests, ArrayInit) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
    "}"
  }));

  Program p = ASTParser(Lexer(in)).parse(); 
  ASSERT_EQ(1, p.fun_defs[0].stmts.size()); 
  VarDeclStmt& s = (VarDeclStmt&)*p.fun_defs[0].stmts[0];
  ASSERT_EQ("int", s.var_def.data_type.type_name); 
  ASSERT_EQ(true, s.var_def.data_type.is_array); 
  ASSERT_EQ("xs", s.var_def.var_name.lexeme());
}

TEST(ASTParserTests, ArrayAssign) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
      "xs[0][1] = 23", 
    "}"
  }));
  
  Program p = ASTParser(Lexer(in)).parse(); 
  AssignStmt& a = (AssignStmt&)*p.fun_defs[0].stmts[1]; 
  ASSERT_EQ("xs", a.lvalue[0].var_name.lexeme()); 

  Expr& e = *a.lvalue[0].array_expr;
  SimpleRValue& v = (SimpleRValue&)*((SimpleTerm&)*e.first).rvalue;
  ASSERT_EQ("0", v.value.lexeme());  

  e = *a.lvalue[0].array_expr_2D;
  v = (SimpleRValue&)*((SimpleTerm&)*e.first).rvalue;
  ASSERT_EQ("1", v.value.lexeme());  

  e = a.expr; 
  v = (SimpleRValue&)*((SimpleTerm&)*e.first).rvalue; 
  ASSERT_EQ("23", v.value.lexeme()); 
}

TEST(ASTParserTests, BadArrayDecl) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [][2]",
    "}"
  }));

  try {
    ASTParser(Lexer(in)).parse();
    FAIL();
  } catch (MyPLException& ex) {
    string msg = ex.what();
    ASSERT_TRUE(msg.starts_with("Parser Error:"));
  }  
}

TEST(ASTParserTests, BadArrayDecl2) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][]",
    "}"
  }));

  try {
    ASTParser(Lexer(in)).parse();
    FAIL();
  } catch (MyPLException& ex) {
    string msg = ex.what();
    ASSERT_TRUE(msg.starts_with("Parser Error:"));
  }  
}

TEST(ASTParserTests, BadArrayAssign) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
      "xs[1][] = 1", 
    "}"
  }));

  try {
    ASTParser(Lexer(in)).parse();
    FAIL();
  } catch (MyPLException& ex) {
    string msg = ex.what();
    cout << msg; 
    ASSERT_TRUE(msg.starts_with("Parser Error:"));
  }  
}

TEST(ASTParserTests, BadArrayAccess) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
      "int x = xs[][]", 
    "}"
  }));

  try {
    ASTParser(Lexer(in)).parse();
    FAIL();
  } catch (MyPLException& ex) {
    string msg = ex.what();
    ASSERT_TRUE(msg.starts_with("Parser Error:"));
  }  
}

//----------------------------------------------------------------------
// Semantic Checker Tests
//----------------------------------------------------------------------

TEST(SemanticCheckerTest, BadArrayDeclExpression) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [k][2]",
    "}"
  }));

  SemanticChecker checker;
  try {
    ASTParser(Lexer(in)).parse().accept(checker);
    FAIL();
  } catch (MyPLException& ex) {
    string msg = ex.what();
    ASSERT_TRUE(msg.starts_with("Static Error:"));
  } 
}

TEST(SemanticCheckerTest, BadArrayAssignExpression) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
      "xs[1][h] = 23",
    "}"
  }));

  SemanticChecker checker;
  try {
    ASTParser(Lexer(in)).parse().accept(checker);
    FAIL();
  } catch (MyPLException& ex) {
    string msg = ex.what();
    ASSERT_TRUE(msg.starts_with("Static Error:"));
  } 
}

TEST(SemanticCheckerTest, BadArrayAccessExpression) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
      "xs[1][1] = 23",
      "int x = xs[gh][1]", 
    "}"
  }));

  SemanticChecker checker;
  try {
    ASTParser(Lexer(in)).parse().accept(checker);
    FAIL();
  } catch (MyPLException& ex) {
    string msg = ex.what();
    ASSERT_TRUE(msg.starts_with("Static Error:"));
  } 
}

TEST(SemanticCheckerTest, Array2DAsArg) {
   stringstream in (build_string({
    "int f (array int xs) {}"
    "void main() {",
      "array int xs = new int [2][2]",
      "f(xs)",
    "}"
  })); 

  SemanticChecker checker;
  ASTParser(Lexer(in)).parse().accept(checker); 
}

TEST(SemanticCheckerTest, BasicArrUsage) {
  stringstream in (build_string({
    "void main() {",
      "array int xs = new int [2][2]",
      "xs[1][1] = 23",
      "int x = xs[1][1]", 
    "}"
  }));  

  SemanticChecker checker;
  ASTParser(Lexer(in)).parse().accept(checker); 
}

//----------------------------------------------------------------------
// VM Tests
//----------------------------------------------------------------------

TEST(VMTests, ArrayAlloc) {
  VMFrameInfo main {"main", 0};
  main.instructions.push_back(VMInstr::PUSH(2));
  main.instructions.push_back(VMInstr::PUSH(2)); 
  main.instructions.push_back(VMInstr::PUSH(0)); 
  main.instructions.push_back(VMInstr::ALLOCA2D()); 
  main.instructions.push_back(VMInstr::WRITE());
  VM vm;
  vm.add(main);
  stringstream out;
  change_cout(out);
  vm.run();
  EXPECT_EQ("2023", out.str());
  restore_cout();
}

TEST(VMTests, ArrayGetIndex) {
  VMFrameInfo main {"main", 0};
  main.instructions.push_back(VMInstr::PUSH(2));
  main.instructions.push_back(VMInstr::PUSH(2)); 
  main.instructions.push_back(VMInstr::PUSH(0)); 
  main.instructions.push_back(VMInstr::ALLOCA2D());  
  main.instructions.push_back(VMInstr::PUSH(1));
  main.instructions.push_back(VMInstr::PUSH(1));
  main.instructions.push_back(VMInstr::GETI2D());
  main.instructions.push_back(VMInstr::WRITE());

  VM vm;
  vm.add(main);
  stringstream out;
  change_cout(out);
  vm.run();
  EXPECT_EQ("0", out.str());
  restore_cout();     
}

TEST(VMTests, ArraySetIndex) {
  VMFrameInfo main {"main", 0};
  main.instructions.push_back(VMInstr::PUSH(2));
  main.instructions.push_back(VMInstr::PUSH(2)); 
  main.instructions.push_back(VMInstr::PUSH(0)); 
  main.instructions.push_back(VMInstr::ALLOCA2D());  
  main.instructions.push_back(VMInstr::STORE(0)); 
  main.instructions.push_back(VMInstr::LOAD(0)); 
  main.instructions.push_back(VMInstr::PUSH(1));
  main.instructions.push_back(VMInstr::PUSH(1));
  main.instructions.push_back(VMInstr::PUSH(23));
  main.instructions.push_back(VMInstr::SETI2D());
  main.instructions.push_back(VMInstr::LOAD(0)); 
  main.instructions.push_back(VMInstr::PUSH(1));
  main.instructions.push_back(VMInstr::PUSH(1));
  main.instructions.push_back(VMInstr::GETI2D());
  main.instructions.push_back(VMInstr::WRITE());

  VM vm;
  vm.add(main);
  stringstream out;
  change_cout(out);
  vm.run();
  EXPECT_EQ("23", out.str());
  restore_cout();     
}

TEST(VMTests, OutOfBoundsIndex) {
  VMFrameInfo main {"main", 0};
  main.instructions.push_back(VMInstr::PUSH(2));
  main.instructions.push_back(VMInstr::PUSH(2)); 
  main.instructions.push_back(VMInstr::PUSH(0)); 
  main.instructions.push_back(VMInstr::ALLOCA2D());    
  main.instructions.push_back(VMInstr::PUSH(2));
  main.instructions.push_back(VMInstr::PUSH(2));
  main.instructions.push_back(VMInstr::GETI2D());  
  VM vm;
  vm.add(main);
  stringstream out;
  change_cout(out);
  try {
    vm.run();
    FAIL();
  } catch(MyPLException& ex) {
    string err = ex.what();
    string msg = "VM Error: out-of-bounds 2D array index";
    msg += " (in main at 6: GETI2D())";
    EXPECT_EQ(msg, err);
  }
  restore_cout();
}

//----------------------------------------------------------------------
// Code Generation Tests
//----------------------------------------------------------------------
TEST(CodeGenerationTests, BasicTest) {
  stringstream in (build_string({
    "void main() {",
    "array int xs = new int[2][4]",
    "xs[1][1] = 3",
    "int x = xs[1][1]",
    "print(x)",
    "}"
  }));
  VM vm;
  CodeGenerator generator(vm);
  ASTParser(Lexer(in)).parse().accept(generator);
  stringstream out;
  change_cout(out);
  vm.run();
  EXPECT_EQ("3", out.str());
  restore_cout();
}   

TEST(CodeGenerationTests, NullInit) {
  stringstream in (build_string({
    "void main() {",
    "array int xs = new int[2][4]", 
    "print(xs[0][0])",
    "}"
  }));

  VM vm; 
  CodeGenerator generator(vm); 
  ASTParser(Lexer(in)).parse().accept(generator); 
  stringstream out; 
  change_cout(out); 
  vm.run(); 
  EXPECT_EQ("null", out.str()); 
  restore_cout(); 
}

TEST(CodeGenerationTests, InvolvedArray) {
  stringstream in (build_string({
    "void main() {", 
    " int count = 0"
    " array int xs = new int[3][5]", 

    " for(int i = 0; i < 3; i = i+ 1) {", 
    "   for(int j = 0; j < 5; j = j+ 1) {", 
    "    xs[i][j] = count", 
    "    count = count + 1", 
    "   }", 
    " }",

    " for(int i = 0; i < 3; i = i+ 1) {", 
    "   for(int j = 0; j < 5; j = j+ 1) {", 
    "     print(xs[i][j])", 
    "     print(\" \")",
    "   }", 
    " }",
    "}"
  })); 

  VM vm; 
  CodeGenerator generator(vm); 
  ASTParser(Lexer(in)).parse().accept(generator); 
  stringstream out; 
  change_cout(out); 
  vm.run(); 
  EXPECT_EQ("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 ", out.str()); 
  restore_cout();   
}

//----------------------------------------------------------------------
// main
//----------------------------------------------------------------------

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

