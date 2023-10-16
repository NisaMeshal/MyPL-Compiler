//----------------------------------------------------------------------
// FILE: print_visitor.cpp
// DATE: CPSC 326, Spring 2023
// AUTH: Nisa Meshal
// DESC: MyPL pretty printer
//----------------------------------------------------------------------

#include "print_visitor.h"
#include <iostream> 

using namespace std;


PrintVisitor::PrintVisitor(ostream& output)
  : out(output)
{
}


void PrintVisitor::inc_indent()
{
  indent += INDENT_AMT;
}


void PrintVisitor::dec_indent()
{
  indent -= INDENT_AMT;
}


void PrintVisitor::print_indent()
{
  out << string(indent, ' ');
}


void PrintVisitor::visit(Program& p)
{
  for (auto struct_def : p.struct_defs)
    struct_def.accept(*this);
  for (auto fun_def : p.fun_defs)
    fun_def.accept(*this);
}

// TODO: Finish the visitor functions

void PrintVisitor::visit(FunDef& f) {
  print_data_type_helper(f.return_type);  

  out << f.fun_name.lexeme() << "(";

  //print params
  if(!f.params.empty()) {
    for (auto param : f.params) {
      if(&param == &f.params.back()) { //reached second from last of params
        break; 
      }

      print_data_type_helper(param.data_type);
      out << param.var_name.lexeme() << ", ";
    }
      //print last param
    print_data_type_helper(f.params.back().data_type);
    out << f.params.back().var_name.lexeme();
  }

  out << ") {" << endl;
   
  inc_indent(); 

  for(auto st : f.stmts) {
    print_indent();
    st->accept(*this); 
    out << endl;
  }
  dec_indent(); 

  out << "}" << endl; 
  out << endl; 
}

void PrintVisitor::visit(StructDef& s) {
  out << "struct " << s.struct_name.lexeme() << " {" << endl;

  if(!s.fields.empty()) {
    inc_indent();
    for (auto field : s.fields) {
      if(&field == &s.fields.back()) {
        break;
      }

      print_indent();
      print_data_type_helper(field.data_type);
      out << field.var_name.lexeme() << "," << endl; 
    }

    print_indent();
    print_data_type_helper(s.fields.back().data_type);
    out << s.fields.back().var_name.lexeme() << endl;

    dec_indent();
  }
  out << "}" << endl << endl;
}

void PrintVisitor::visit(ReturnStmt& s) {
  out << "return ";
  s.expr.accept(*this);  
}

void PrintVisitor::visit(WhileStmt& s) {
  out << "while (";
  s.condition.accept(*this);
  out << ") {" << endl;

  inc_indent(); 
  for(auto stmt : s.stmts) {
    print_indent(); 
    stmt->accept(*this); 
    out << endl; 
  }
  dec_indent(); 
  
  print_indent(); 
  out << "}" << endl;
}

void PrintVisitor::visit(ForStmt& s) {
  out << "for ("; 
  s.var_decl.accept(*this); 
  out << "; ";
  s.condition.accept(*this); 
  out << "; ";
  s.var_decl.accept(*this);
  out << ") {" << endl; 

  inc_indent();
  for(auto stmt : s.stmts) {
    print_indent();
    stmt->accept(*this);
    out << endl;
  }
  dec_indent();

  print_indent();
  out << "}" << endl;
}

void PrintVisitor::visit(IfStmt& s) {
  out << "if (";
  s.if_part.condition.accept(*this); 
  out << ") {" << endl; 
  
  inc_indent();
  for(auto stmt : s.if_part.stmts) {
    print_indent();
    stmt->accept(*this); 
    out << endl;
  }
  dec_indent(); 
  
  print_indent();
  out << "}" << endl; 

  if(!s.else_ifs.empty()) {
    for(auto elif : s.else_ifs) {
      print_indent();
      out << "elseif (";
      elif.condition.accept(*this);
      out << ") {" << endl ;
      
      inc_indent();
      for(auto stmt : elif.stmts) {
        print_indent();
        stmt->accept(*this); 
        out << endl; 
      }
      dec_indent();

      print_indent();
      out << "}" << endl;
    }
  }

  if(!s.else_stmts.empty()) {
    print_indent();
    out << "else {" << endl;

    inc_indent(); 
    for(auto stmt : s.else_stmts) {  
      print_indent();
      stmt->accept(*this);
      out << endl; 
    }
    dec_indent();

    print_indent();
    out << "}";
  }
}

void PrintVisitor::visit(VarDeclStmt& s) {
  print_data_type_helper(s.var_def.data_type);
  out << s.var_def.var_name.lexeme() << " = ";
  s.expr.accept(*this); 
}

void PrintVisitor::visit(AssignStmt& s) {
  for(auto lval  : s.lvalue) {
    out << lval.var_name.lexeme();
    if(lval.array_expr.has_value()) {
      out << "[";
      lval.array_expr->accept(*this); 
      out << "]"; 
      if(lval.array_expr_2D.has_value()) {
        out << "[";
        lval.array_expr_2D->accept(*this);
        out << "]"; 
      }
    }
    if(lval.var_name.lexeme() != s.lvalue.front().var_name.lexeme() && lval.var_name.lexeme() != s.lvalue.back().var_name.lexeme()) {
      out << ".";
    }
  }

  out << " = ";
  s.expr.accept(*this);
}

void PrintVisitor::visit(CallExpr& e) {
  out << e.fun_name.lexeme() << "(";

  if(e.args.size() > 1) {
    for(auto arg : e.args) {
      if(&arg == &e.args.back()) {
        break;
      }

      arg.accept(*this);
      out << ", ";
    }
  }

  e.args.back().accept(*this);
  out << ")"; 
}

void PrintVisitor::visit(Expr& e) {
  e.first->accept(*this);

  if(e.op != nullopt) {
    Token t = *e.op; 
    out << " " << t.lexeme() << " "; 
  }

  if(e.rest != nullptr) {
    e.rest->accept(*this); 
  }
}

void PrintVisitor::visit(SimpleTerm& t) {
  if(t.first_token().type() == TokenType::NOT) {
    out << "not ";
  }

  t.rvalue->accept(*this); 
}

void PrintVisitor::visit(ComplexTerm& t) {
  out << "(";
  t.expr.accept(*this); 
  out << ")"; 
}

void PrintVisitor::visit(SimpleRValue& v) {
  if(v.value.type() == TokenType::STRING_VAL) {
    out << "\"" << v.value.lexeme() << "\"";
  }
  else {
      out << v.value.lexeme() ; 
  }
}

void PrintVisitor::visit(NewRValue& v) {
  out << "new " << v.type.lexeme();
  if(v.array_expr.has_value()) {
    out << "[";
    v.array_expr->accept(*this); 
    out << "]"; 
    if(v.array_expr_2D.has_value()) {
      out << "[";
      v.array_expr_2D->accept(*this);
      out << "]";
    }
  }
}

void PrintVisitor::visit(VarRValue& v) {
  for(auto var : v.path) {
    out << var.var_name.lexeme();

    if(var.array_expr.has_value()) {
      out << "[";
      var.array_expr->accept(*this); 
      out << "]";
      
      if(var.array_expr_2D.has_value()) {
        out <<"[";
        var.array_expr_2D->accept(*this);
        out << "]";
      }
    }

    if(var.var_name.lexeme() != v.path[0].var_name.lexeme() && var.var_name.lexeme() != v.path.back().var_name.lexeme()) {
      out << ".";
    }
  }
} 


//-------------------HELPERS------------------------

void PrintVisitor::print_data_type_helper(DataType& d) {
  if(d.is_array == true) {
    out << "array ";
  }

  out << d.type_name << " ";
}

bool PrintVisitor::compare(TokenType t) {
  if(t == TokenType::INT_VAL ||
      t == TokenType::DOUBLE_VAL ||
      t == TokenType::BOOL_VAL ||
      t == TokenType::CHAR_VAL ||
      t == TokenType::STRING_VAL ||
      t == TokenType::NULL_VAL ||
      t == TokenType::ID) {
    return true; 
  }

  return false;
}