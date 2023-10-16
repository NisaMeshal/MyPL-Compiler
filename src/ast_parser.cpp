//----------------------------------------------------------------------
// FILE: ast_parser.cpp
// DATE: CPSC 326, Spring 2023
// AUTH: Nisa Meshal
// DESC: This is a parser that formats everything read into an AST
//----------------------------------------------------------------------

#include "ast_parser.h"

using namespace std;


ASTParser::ASTParser(const Lexer& a_lexer)
  : lexer {a_lexer}
{}


void ASTParser::advance()
{
  curr_token = lexer.next_token();
}


void ASTParser::eat(TokenType t, const string& msg)
{
  if (!match(t))
    error(msg);
  advance();
}


bool ASTParser::match(TokenType t)
{
  return curr_token.type() == t;
}


bool ASTParser::match(initializer_list<TokenType> types)
{
  for (auto t : types)
    if (match(t))
      return true;
  return false;
}


void ASTParser::error(const string& msg)
{
  string s = msg + " found '" + curr_token.lexeme() + "' ";
  s += "at line " + to_string(curr_token.line()) + ", ";
  s += "column " + to_string(curr_token.column());
  throw MyPLException::ParserError(s);
}


bool ASTParser::bin_op()
{
  return match({TokenType::PLUS, TokenType::MINUS, TokenType::TIMES,
      TokenType::DIVIDE, TokenType::AND, TokenType::OR, TokenType::EQUAL,
      TokenType::LESS, TokenType::GREATER, TokenType::LESS_EQ,
      TokenType::GREATER_EQ, TokenType::NOT_EQUAL});
}

Program ASTParser::parse()
{
  Program p;
  advance();
  while (!match(TokenType::EOS)) {
    if (match(TokenType::STRUCT))
      struct_def(p);
    else
      fun_def(p);
  }
  eat(TokenType::EOS, "expecting end-of-file");
  return p;
}


//----STRUCT ID LBRACE <fields> RBRACE---
void ASTParser::struct_def(Program& p){
  StructDef sd; 

  eat(TokenType::STRUCT, "expecting struct");

  sd.struct_name = curr_token; 
  eat(TokenType::ID, "expecting ID in struct def");

  eat(TokenType::LBRACE, "expecting '{' in struct def");

  if(match(TokenType::RBRACE)){ //empty struct
    eat(TokenType::RBRACE, "expecting '}' in struct def");
  }
  else{
    fields(sd); // pass in StructDef so we can attatch the fields
    eat(TokenType::RBRACE, "expecting '}' in struct fields");
  }

  p.struct_defs.push_back(sd); //push the struct into program structs
}

//----( <data_type> | VOID_TYPE ) ID LPAREN <params> RPAREN LBRACE ( <stmt> )∗ RBRACE----
void ASTParser::fun_def(Program& p){
  FunDef fd; 

  //return type
  if(match(TokenType::VOID_TYPE)){
    DataType dt; 
    dt.type_name = curr_token.lexeme();
    advance(); //go past the void
    fd.return_type = dt; 
  }
  else{ //has to be base type or array
    data_type(fd.return_type);
  }

  //function name
  fd.fun_name = curr_token; 
  eat(TokenType::ID, "expecting function name");

  //parameters
  eat(TokenType::LPAREN, "expecting '(' in fun_def");
  params(fd); 
  eat(TokenType::RPAREN, "expenting ')' in fun_def");

  //function body
  eat(TokenType::LBRACE, "expecting '{' in fun_def");
  while(!match(TokenType::RBRACE)) {
    stmt(fd.stmts);
  }
  eat(TokenType::RBRACE, "expecting '}' in fun def");

  p.fun_defs.push_back(fd);
}

//---- <data_type> ID ( COMMA <data_type> ID )∗ | ϵ ----
void ASTParser::fields(StructDef& s) {
  VarDef vd; 

  //modify the new VarDef
  data_type(vd.data_type);
  vd.var_name = curr_token; 
  eat(TokenType::ID, "expecting id in fields"); 

  s.fields.push_back(vd);

  if(match(TokenType::COMMA)) {
    while(match(TokenType::COMMA)) {
      advance(); 

      VarDef new_var_def; 
      data_type(new_var_def.data_type);
      new_var_def.var_name = curr_token;
      eat(TokenType::ID, "expecting id in fields recursion");

      s.fields.push_back(new_var_def);
    }
  }
}

//---- <data_type> ID ( COMMA <data_type> ID )∗ | ϵ ----
void ASTParser::params(FunDef& f)
{
  while(!match(TokenType::RPAREN))
  {
    VarDef vd; 

    data_type(vd.data_type);
    vd.var_name = curr_token; 
    eat(TokenType::ID, "expecting id in params");

    f.params.push_back(vd);

    if(match(TokenType::COMMA)) {
      advance(); 

      if(match(TokenType::RPAREN)) {
        error("too many commas in params"); 
      }
    }
  }
}

//----<base_type> | ID | ARRAY ( <base_type> | ID ) ----
void ASTParser::data_type(DataType& dt)
{
  if(match(TokenType::ID)) //the type name is an id
  {
    dt.type_name = curr_token.lexeme();
    advance();
  }
  else if (match(TokenType::ARRAY))
  {
    advance(); //pass the array

    dt.is_array = true; 
    dt.type_name = curr_token.lexeme();

    advance(); //pass the array type
  }
  else
  {
    if(base_type())
    {
      dt.type_name = curr_token.lexeme();
      advance();
    }
  }
}

//matches with a base token type
bool ASTParser::base_type() 
{
  return match({TokenType::STRING_TYPE, TokenType::DOUBLE_TYPE, TokenType::INT_TYPE, TokenType::BOOL_TYPE, TokenType::CHAR_TYPE});
}


//----------------------------------------------------------------------
// Statement-related functions
//----------------------------------------------------------------------

//accepts a vector of shared pointers since this is used in a few different cases
//----<vdecl_stmt> | <assign_stmt> | <if_stmt> | <while_stmt> | <for_stmt> | <call_expr> | <ret_stmt>
void ASTParser::stmt(std::vector<std::shared_ptr<Stmt>>& s){

  if(match(TokenType::ID)) {

    Token id = curr_token; //save the current id token
    advance(); //curr_token should now equal ( or [

    if(match(TokenType::LPAREN)) { //call expression
      std::shared_ptr<CallExpr> ce = make_shared<CallExpr>(call_expr(id));
      s.push_back(ce);
    }
    else if (match(TokenType::ID)) { //vdecl statement
      //my_struct x2 = null
      
      std::shared_ptr<VarDeclStmt> vdec = make_shared<VarDeclStmt>(vdecl_stmt(id)); 
      s.push_back(vdec); 

    }
    else { //assign statement
      AssignStmt new_assign;

      lvalue(new_assign.lvalue, id); 
      assign_stmt(new_assign);

      std::shared_ptr<AssignStmt> as = make_shared<AssignStmt>(new_assign); 

      s.push_back(as); 
    }

  }
  else if (match(TokenType::RETURN)) { //return statement

    advance(); 
    std::shared_ptr<ReturnStmt> st = make_shared<ReturnStmt>(ret_stmt());
    s.push_back(st);

  }
  else if (match(TokenType::IF)) { //if statements

    advance(); //eats the if
    BasicIf bi = if_stmt();

    //encounter else or else if cases
    if(match({TokenType::ELSE, TokenType::ELSEIF})) { 
      IfStmt new_if; 
      new_if.if_part = bi;  

      while(match(TokenType::ELSEIF)) {
        advance();
        BasicIf basic_new_if = if_stmt();
        new_if.else_ifs.push_back(basic_new_if);
      }

      if(match(TokenType::ELSE)) {
        advance();
        
        eat(TokenType::LBRACE, "expecting '{' in else");
        while(!match(TokenType::RBRACE)) {
          stmt(new_if.else_stmts);
        }       
        eat(TokenType::RBRACE, "expecting '}' in else");
      }

      std::shared_ptr<IfStmt> ptr = make_shared<IfStmt>(new_if);
      s.push_back(ptr);
    } 
    //simple if statement
    else { 
      std::shared_ptr<IfStmt> is = make_shared<IfStmt>();
      is->if_part = bi;
      s.push_back(is); 
    }

  }
  else if (match(TokenType::WHILE)) { //while statements

    advance(); 
    std::shared_ptr<WhileStmt> ws = make_shared<WhileStmt>(while_stmt());
    s.push_back(ws);

  }
  else if (match(TokenType::FOR)) { //for statements

    advance(); 
    std::shared_ptr<ForStmt> fs = make_shared<ForStmt>(for_stmt());
    s.push_back(fs);

  }
  else { //if any of the above didn't
    std::shared_ptr<VarDeclStmt> vds = make_shared<VarDeclStmt>(vdecl_stmt());
    s.push_back(vds);
  }
}


//---  RETURN <expr> ---
ReturnStmt ASTParser::ret_stmt() {
  ReturnStmt rs; 
  expr(rs.expr);

  return rs; 
}


//--- WHILE LPAREN <expr> RPAREN LBRACE ( <stmt> )∗ RBRACE ---
WhileStmt ASTParser::while_stmt() {
  WhileStmt ws; 

  eat(TokenType::LPAREN, "expecting '(' in while stmt");
  expr(ws.condition);
  eat(TokenType::RPAREN, "expecting ')' in while stmt");

  eat(TokenType::LBRACE, "expecting '{' in while stmt");
  while(!match(TokenType::RBRACE)){
    stmt(ws.stmts);
  }
  eat(TokenType::RBRACE, "expecting '}' in while stmt");

  return ws; 
}

//---  <data_type> ID ASSIGN <expr> ---
VarDeclStmt ASTParser::vdecl_stmt() {
  VarDeclStmt vds;

  //data type
  data_type(vds.var_def.data_type);

  //id
  vds.var_def.var_name = curr_token;
  eat(TokenType::ID, "expecting id in vdecl");
  //assign
  eat(TokenType::ASSIGN, "expecting assign in vdecl");
  //expression
  expr(vds.expr); 

  return vds; 
}

//overloaded vdecl_stmt because i can
VarDeclStmt ASTParser::vdecl_stmt(Token id) {
  VarDeclStmt vds; 

  DataType dt;
  dt.type_name = id.lexeme();

  //set up the var_def
  vds.var_def.data_type = dt;
  vds.var_def.var_name = curr_token;

  eat(TokenType::ID, "expecting id in vdecl overload"); 
  eat(TokenType::ASSIGN, "expecting '=' in vdecl overload");
  expr(vds.expr);

  return vds; 
}

//--- <lvalue> ASSIGN <expr> ---
void ASTParser::assign_stmt(AssignStmt& s) {

  eat(TokenType::ASSIGN, "expecting = in assign");
  expr(s.expr);
}


//--- FOR LPAREN <vdecl_stmt> SEMICOLON <expr> SEMICOLON <assign_stmt> <RPAREN> LBRACE ( <stmt> )∗ RBRACE ---
ForStmt ASTParser::for_stmt() {
  ForStmt fs;

  //parenthesis conditions
  eat(TokenType::LPAREN, "expecting '(' in for");

  //int i = 0
  fs.var_decl = vdecl_stmt();
  eat(TokenType::SEMICOLON, "expecting ';' in for");

  //i < 5
  expr(fs.condition);
  eat(TokenType::SEMICOLON, "expecting ';' in for");

  //i = i + 1
  AssignStmt as; 

  VarRef vr; 
  vr.var_name = curr_token;
  advance(); 
  as.lvalue.push_back(vr);

  assign_stmt(as); 
  fs.assign_stmt = as;
  eat(TokenType::RPAREN, "expecting ')' in for");

  //body
  eat(TokenType::LBRACE, "expecting '{' in for");
  while(!match(TokenType::RBRACE)) {
    stmt(fs.stmts);
  }
  eat(TokenType::RBRACE, "expecting '}' in for");

  return fs; 
}


//--- IF LPAREN <expr> RPAREN LBRACE ( <stmt> )∗ RBRACE <if_stmt_t> ---
BasicIf ASTParser::if_stmt() {
  BasicIf bi; 

  eat(TokenType::LPAREN, "expectin '(' in if");
  expr(bi.condition);
  eat(TokenType::RPAREN, "expecting ')' in if");

  //body
  eat(TokenType::LBRACE, "expecting '{' in if");
  while(!match(TokenType::RBRACE)) {
    stmt(bi.stmts);
  }
  eat(TokenType::RBRACE, "expecting '}' in if");

  return bi; 
}


//--- ELSEIF LPAREN <expr> RPAREN LBRACE ( <stmt> )∗ RBRACE <if_stmt_t> | ELSE LBRACE ( <stmt> )∗ RBRACE | ϵ ---
IfStmt ASTParser::if_stmt_t(BasicIf bi) {
  IfStmt is; 

  is.if_part = bi;
  
  if(match(TokenType::ELSEIF)) {
    advance(); //handles elseif token
    is.else_ifs.push_back(if_stmt());
  }
  else { //else case
    advance(); 
    eat(TokenType::LBRACE, "expecting '{' in else");

    while(!match(TokenType::RBRACE)) {
      stmt(is.else_stmts);
    }

    eat(TokenType::RBRACE, "expecting '}' in else"); 
  }

  return is; 
}

//--- ID LPAREN ( <expr> ( COMMA <expr> )∗ | ϵ ) RPAREN ---
CallExpr ASTParser::call_expr(Token id) {
  CallExpr ce; 
  ce.fun_name = id; 

  eat(TokenType::LPAREN, "expecting '(' in call expr"); 
  while(!match(TokenType::RPAREN)) {
    Expr e; 
    expr(e);
    ce.args.push_back(e);
    if(match(TokenType::COMMA)) {
      advance(); 
    }
  }

  eat(TokenType::RPAREN, "expecting ')' in call expr"); 

  return ce; 
}


//--- ID ( LBRACKET <expr> RBRACKET | ϵ ) ( DOT ID ( LBRACKET <expr> RBRACKET | ϵ ) )∗ ---
void ASTParser::lvalue(std::vector<VarRef>& v, Token id) { //!!!!!!
  VarRef vr;
  //we should start sitting at dot or lbracket, id was already ate in stmt
  vr.var_name = id; 

  if(match(TokenType::LBRACKET)) {
    advance(); //pass the lbracket

    Expr e; 
    expr(e);
    vr.array_expr = e; 

    eat(TokenType::RBRACKET, "expecting ']' in lval");

    if(match(TokenType::LBRACKET)) {
      advance();
      Expr expr2d; 
      expr(expr2d);
      vr.array_expr_2D = expr2d;

      eat(TokenType::RBRACKET, "expecting ']' in 2d array lval");
    }
  }

  v.push_back(vr);

  while(match(TokenType::DOT)) {
    advance(); //pass the dot

    VarRef new_var; 

    new_var.var_name = curr_token; 
    eat(TokenType::ID, "expecting id in recursive lval"); 

    if(match(TokenType::LBRACKET)) {
      advance();
      Expr e; 
      expr(e);
      new_var.array_expr = e; 
      eat(TokenType::RBRACKET, "expecting ']' in lval");

      if(match(TokenType::LBRACKET)) {
        advance(); 
        Expr expr2d; 
        expr(expr2d);
        new_var.array_expr_2D = expr2d; 

        eat(TokenType::RBRACKET, "expecting ']' in 2d array lval");
      }
    }

    v.push_back(new_var); 
  }    
}

//----------------------------------------------------------------------
// Expression-related functions
//----------------------------------------------------------------------


//--- ( <rvalue> | NOT <expr> | LPAREN <expr> RPAREN ) ( <bin_op> <expr> | ϵ ) ---
void ASTParser::expr(Expr& e) {

  if(match(TokenType::NOT)) { 

    e.negated = true; 
    advance();
    
    expr(e);  
  }
  else if (match(TokenType::LPAREN)) {
    advance(); 
    
    ComplexTerm new_complex;
    expr(new_complex.expr);

    std::shared_ptr<ComplexTerm> ptr = make_shared<ComplexTerm>(new_complex);
    e.first = ptr; 

    eat(TokenType::RPAREN, "expecting ')' in expr");
  }
  else {
    rvalue(e); 
  }

  if(bin_op()) {
    e.op = curr_token; 
    advance();
    
    Expr new_exp; 
    expr(new_exp);
    std::shared_ptr<Expr> ptr = make_shared<Expr>(new_exp);
    e.rest = ptr; 
  }
}


//--- <base_rvalue> | NULL_VAL | <new_rvalue> | <var_rvalue> | <call_expr> ---
void ASTParser::rvalue(Expr& e) {
  SimpleTerm st; 

  if(base_rvalue() || match(TokenType::NULL_VAL)) {
    SimpleRValue sr; 

    sr.value = curr_token; 
    std::shared_ptr<SimpleRValue> simple_rval_ptr = make_shared<SimpleRValue>(sr);
    st.rvalue = simple_rval_ptr; 

    advance();
  }
  else if (match(TokenType::NEW)) {
    std::shared_ptr<NewRValue> new_rval = make_shared<NewRValue>(new_rvalue());

    st.rvalue = new_rval;
  }
  else {
    //currtoken is id
    Token id = curr_token; 
    advance(); 

    if(match(TokenType::LPAREN)) {

      std::shared_ptr<CallExpr> new_call_e = make_shared<CallExpr>(call_expr(id));

      st.rvalue = new_call_e;
    }
    else {
      std::shared_ptr<VarRValue> new_var_rval = make_shared<VarRValue>(var_rvalue(id)); 

      st.rvalue = new_var_rval;
    }
  }

  std::shared_ptr<SimpleTerm> simple_ptr = make_shared<SimpleTerm>(st);

  e.first = simple_ptr; 
}


//--- INT_VAL | DOUBLE_VAL | BOOL_VAL | CHAR_VAL | STRING_VAL ---
bool ASTParser::base_rvalue() {
  return match({TokenType::INT_VAL, TokenType::DOUBLE_VAL, TokenType::STRING_VAL, TokenType::BOOL_VAL, TokenType::CHAR_VAL});
}


//---  NEW ID ( LBRACKET <expr> RBRACKET | ϵ ) | NEW <base_type> LBRACKET <expr> RBRACKET ---
NewRValue ASTParser::new_rvalue() {
  advance(); //eat the new

  NewRValue new_rval; 

  if(match(TokenType::ID)) {
    new_rval.type = curr_token;
    advance();

    if(match(TokenType::LBRACKET)) {
      advance();
      
      Expr e; 
      expr(e);
      new_rval.array_expr = e; 
      eat(TokenType::RBRACKET, "expecting ']' in new rval");

      if(match(TokenType::LBRACKET)) {
        advance();

        Expr array2d; 
        expr(array2d);
        new_rval.array_expr_2D = array2d;

        eat(TokenType::RBRACKET, "expecting ']' in new 2d array rval");
      }
    }
  }
  else if(base_type()) {
    new_rval.type = curr_token;
    advance();

    eat(TokenType::LBRACKET, "expecting '[' in new rval");

    Expr e; 
    expr(e);
    new_rval.array_expr = e; 
    
    eat(TokenType::RBRACKET, "expecting ']' in new rval");

    if(match(TokenType::LBRACKET)) {
      advance();  

      Expr array2d; 
      expr(array2d);
      new_rval.array_expr_2D = array2d;

      eat(TokenType::RBRACKET, "expecting ']' in new 2d array rval");
    }
  }

  return new_rval; 
}


//--- ID ( LBRACKET <expr> RBRACKET | ϵ ) ( DOT ID ( LBRACKET <expr> RBRACKET | ϵ ) )∗ --- 
VarRValue ASTParser::var_rvalue(Token id) {
  //id was already ate in the caller, so we're on dot or id, or it's empty
  VarRValue vrv; 

  VarRef vr; 
  vr.var_name = id; 
  if(match(TokenType::LBRACKET)) {
    advance();
    
    Expr e; 
    expr(e);
    vr.array_expr = e; 
    eat(TokenType::RBRACKET, "expecting ']' in var rval"); 

    if(match(TokenType::LBRACKET)) {
      advance();

      Expr array2d; 
      expr(array2d);
      vr.array_expr_2D = array2d;

      eat(TokenType::RBRACKET, "expecting ']' in new 2d array rval");
    }
  }
  vrv.path.push_back(vr); 

  while(match(TokenType::DOT)) {
    VarRef new_ref; 

    advance(); //pass the dot
    new_ref.var_name = curr_token; 
    eat(TokenType::ID, "expecting ID in var rval");

    if(match(TokenType::LBRACKET)) {
      advance();

      Expr e; 
      expr(e);
      new_ref.array_expr = e; 
      eat(TokenType::RBRACKET, "expecting ']' in var rval"); 

      if(match(TokenType::LBRACKET)) {
        advance();

        Expr array2d; 
        expr(array2d);
        new_ref.array_expr_2D = array2d;

        eat(TokenType::RBRACKET, "expecting ']' in new 2d array rval");
      }
    }

    vrv.path.push_back(new_ref);     
  }

  return vrv; 
}