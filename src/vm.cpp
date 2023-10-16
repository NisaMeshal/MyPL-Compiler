//----------------------------------------------------------------------
// FILE: vm.cpp
// DATE: CPSC 326, Spring 2023
// AUTH: Nisa Meshal  
// DESC: VM instructions
//----------------------------------------------------------------------

#include <iostream>
#include "vm.h"
#include "mypl_exception.h"


using namespace std;


void VM::error(string msg) const
{
  throw MyPLException::VMError(msg);
}


void VM::error(string msg, const VMFrame& frame) const
{
  int pc = frame.pc - 1;
  VMInstr instr = frame.info.instructions[pc];
  string name = frame.info.function_name;
  msg += " (in " + name + " at " + to_string(pc) + ": " +
    to_string(instr) + ")";
  throw MyPLException::VMError(msg);
}


string to_string(const VM& vm)
{
  string s = "";
  for (const auto& entry : vm.frame_info) {
    const string& name = entry.first;
    s += "\nFrame '" + name + "'\n";
    const VMFrameInfo& frame = entry.second;
    for (int i = 0; i < frame.instructions.size(); ++i) {
      VMInstr instr = frame.instructions[i];
      s += "  " + to_string(i) + ": " + to_string(instr) + "\n"; 
    }
  }
  return s;
}


void VM::add(const VMFrameInfo& frame)
{
  frame_info[frame.function_name] = frame;
}


void VM::run(bool DEBUG)
{
  // grab the "main" frame if it exists
  if (!frame_info.contains("main"))
    error("No 'main' function");
  shared_ptr<VMFrame> frame = make_shared<VMFrame>();
  frame->info = frame_info["main"];
  call_stack.push(frame);

  // run loop (keep going until we run out of instructions)
  while (!call_stack.empty() and frame->pc < frame->info.instructions.size()) {

    // get the next instruction
    VMInstr& instr = frame->info.instructions[frame->pc];

    // increment the program counter
    ++frame->pc;

    // for debugging
    if (DEBUG) {
      // TODO
      cerr << endl << endl;
      cerr << "\t FRAME.........: " << frame->info.function_name << endl;
      cerr << "\t PC............: " << (frame->pc - 1) << endl;
      cerr << "\t INSTR.........: " << to_string(instr) << endl;
      cerr << "\t NEXT OPERAND..: ";
      if (!frame->operand_stack.empty())
        cerr << to_string(frame->operand_stack.top()) << endl;
      else
        cerr << "empty" << endl;
      cerr << "\t NEXT FUNCTION.: ";
      if (!call_stack.empty())
        cerr << call_stack.top()->info.function_name << endl;
      else
        cerr << "empty" << endl;
    }

    //----------------------------------------------------------------------
    // Literals and Variables
    //----------------------------------------------------------------------

    if (instr.opcode() == OpCode::PUSH) {
      frame->operand_stack.push(instr.operand().value());
    }

    else if (instr.opcode() == OpCode::POP) {
      frame->operand_stack.pop();
    }

    // TODO: Finish LOAD and STORE

    else if (instr.opcode() == OpCode::LOAD) {
      frame->operand_stack.push(frame->variables[get<int>(instr.operand().value())]); 
    }

    else if (instr.opcode() == OpCode::STORE) {  
      if(!frame->variables.empty()) {
        if(get<int>(instr.operand().value()) >= frame->variables.size()) {
          frame->variables.push_back(frame->operand_stack.top());
        }
        else {
          frame->variables[get<int>(instr.operand().value())] = frame->operand_stack.top();
        } 
      }
      else {
        frame->variables.push_back(frame->operand_stack.top());
      }

      frame->operand_stack.pop(); 
    }


    //----------------------------------------------------------------------
    // Operations
    //----------------------------------------------------------------------

    else if (instr.opcode() == OpCode::ADD) {
      VMValue x = frame->operand_stack.top();
      ensure_not_null(*frame, x);
      frame->operand_stack.pop();
      VMValue y = frame->operand_stack.top();
      ensure_not_null(*frame, y);
      frame->operand_stack.pop();
      frame->operand_stack.push(add(y, x));
    }

    // TODO: Finish SUB, MUL, DIV, AND, OR, NOT, COMPLT, COMPLE,
    // CMPGT, CMPGE, CMPEQ, CMPNE
    
    else if (instr.opcode() == OpCode::SUB) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      ensure_not_null(*frame, y); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(sub(y, x)); 
    }

    else if (instr.opcode() == OpCode::MUL) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      ensure_not_null(*frame, y); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(mul(y, x)); 
    }

    else if (instr.opcode() == OpCode::DIV) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      ensure_not_null(*frame, y); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(div(y, x)); 
    }

    else if (instr.opcode() == OpCode::AND) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      ensure_not_null(*frame, y); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(get<bool>(y) && get<bool>(x)); 
    }

    else if (instr.opcode() == OpCode::OR) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      ensure_not_null(*frame, y); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(get<bool>(y) || get<bool>(x)); 
    }

    else if (instr.opcode() == OpCode::NOT) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      frame->operand_stack.push(!get<bool>(x)); 
    }    
    
    else if (instr.opcode() == OpCode::CMPLT) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      ensure_not_null(*frame, y); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(get<bool>(lt(y, x))); 
    }

    else if (instr.opcode() == OpCode::CMPLE) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      ensure_not_null(*frame, y); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(get<bool>(le(y, x))); 
    }

    else if (instr.opcode() == OpCode::CMPGT) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      ensure_not_null(*frame, y); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(get<bool>(gt(y, x))); 
    }

    else if (instr.opcode() == OpCode::CMPGE) {
      VMValue x = frame->operand_stack.top(); 
      ensure_not_null(*frame, x);
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      ensure_not_null(*frame, y); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(get<bool>(ge(y, x))); 
    }

    else if (instr.opcode() == OpCode::CMPEQ) {
      VMValue x = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(get<bool>(eq(y, x))); 
    }

    else if (instr.opcode() == OpCode::CMPNE) {
      VMValue x = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      VMValue y = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      frame->operand_stack.push(!get<bool>(eq(y, x))); 
    }

    //----------------------------------------------------------------------
    // Branching
    //----------------------------------------------------------------------

    // TODO: Finish JMP and JMPF
    else if (instr.opcode() == OpCode::JMP) {
      frame->pc = get<int>(instr.operand().value());
    }

    else if (instr.opcode() == OpCode::JMPF) {
      if(!get<bool>(frame->operand_stack.top())) {
        frame->pc = get<int>(instr.operand().value()); 
      }
      frame->operand_stack.pop(); 
    }
    
    //----------------------------------------------------------------------
    // Functions
    //----------------------------------------------------------------------


    // TODO: Finish CALL, RET
    else if (instr.opcode() == OpCode::CALL) {
      string g = get<string>(instr.operand().value()); 
      shared_ptr<VMFrame> new_frame = make_shared<VMFrame>();
      new_frame->info = frame_info[g]; 
      call_stack.push(new_frame); 
      for (int i = 0; i < new_frame->info.arg_count; ++i) {
        VMValue x = frame->operand_stack.top(); 
        new_frame->operand_stack.push(x); 
        frame->operand_stack.pop();       
      }

      frame = new_frame; 
    }
    
    else if (instr.opcode() == OpCode::RET) {
      VMValue v = frame->operand_stack.top(); 
      call_stack.pop(); 
      if(call_stack.size() != 0) {
        frame = call_stack.top(); 
        frame->operand_stack.push(v); 
      }
    }

    //----------------------------------------------------------------------
    // Built in functions
    //----------------------------------------------------------------------


    else if (instr.opcode() == OpCode::WRITE) {
      VMValue x = frame->operand_stack.top();
      frame->operand_stack.pop();
      cout << to_string(x);
    }

    else if (instr.opcode() == OpCode::READ) {
      string val = "";
      getline(cin, val);
      frame->operand_stack.push(val);
    }

    // TODO: Finish SLEN, ALEN, GETC, TODBL, TOSTR, CONCAT
    
    else if (instr.opcode() == OpCode::SLEN) {
      ensure_not_null(*frame, frame->operand_stack.top());
      int x = get<string>(frame->operand_stack.top()).size();
      frame->operand_stack.pop();
      frame->operand_stack.push(x); 
    }

    else if (instr.opcode() == OpCode::ALEN) {
      ensure_not_null(*frame, frame->operand_stack.top()); 
      int x = array_heap[get<int>(frame->operand_stack.top())].size(); 
      frame->operand_stack.pop();
      frame->operand_stack.push(x); 
    }

    else if (instr.opcode() == OpCode::GETC) {
      ensure_not_null(*frame, frame->operand_stack.top()); 
      string x = get<string>(frame->operand_stack.top()); 
      frame->operand_stack.pop();
      ensure_not_null(*frame, frame->operand_stack.top()); 
      int y = get<int>(frame->operand_stack.top()); 
      frame->operand_stack.pop();  

      if(y >= x.length()) {
        error("out-of-bounds string index", *frame); 
      }

      frame->operand_stack.push(string(1, x[y])); 
    }

    else if (instr.opcode() == OpCode::TOINT) {
      ensure_not_null(*frame, frame->operand_stack.top()); 
      VMValue x = frame->operand_stack.top(); 
      frame->operand_stack.pop();

      if(holds_alternative<double>(x)) {
        frame->operand_stack.push(int(get<double>(x))); 
      }
      else {
        if(isdigit(get<string>(x)[0])) {
          frame->operand_stack.push(stoi(get<string>(x))); 
        }
        else {
          error("cannot convert string to int", *frame); 
        }
      }
    }

    else if (instr.opcode() == OpCode::TODBL) {
      ensure_not_null(*frame, frame->operand_stack.top()); 
      VMValue x = frame->operand_stack.top(); 
      frame->operand_stack.pop();

      if(holds_alternative<int>(x)) {
        frame->operand_stack.push(double(get<int>(x))); 
      }
      else {
        if(isdigit(get<string>(x)[0])) {
          frame->operand_stack.push(stod(get<string>(x))); 
        }
        else {
          error("cannot convert string to double", *frame); 
        }
      }
    }

    else if (instr.opcode() == OpCode::TOSTR) {
      ensure_not_null(*frame, frame->operand_stack.top()); 
      string x = to_string(frame->operand_stack.top());
      frame->operand_stack.pop();
      frame->operand_stack.push(x); 
    }
    
    else if (instr.opcode() == OpCode::CONCAT) {
      ensure_not_null(*frame, frame->operand_stack.top());
      string x = get<string>(frame->operand_stack.top()); 
      frame->operand_stack.pop(); 
      ensure_not_null(*frame, frame->operand_stack.top()); 
      string y = get<string>(frame->operand_stack.top());
      frame->operand_stack.pop(); 

      frame->operand_stack.push(y + x); 
    }
    //----------------------------------------------------------------------
    // heap
    //----------------------------------------------------------------------

    // TODO: Finish ALLOCS, ALLOCA, ADDF, SETF, GETF, SETI, GETI

    else if (instr.opcode() == OpCode::ALLOCS) {
      struct_heap[next_obj_id] = {};
      frame->operand_stack.push(next_obj_id);
      next_obj_id++;
    }

    else if (instr.opcode() == OpCode::ALLOCA) {
      VMValue x = frame->operand_stack.top();
      frame->operand_stack.pop();
      int y = get<int>(frame->operand_stack.top());
      frame->operand_stack.pop();
      array_heap[next_obj_id] = vector<VMValue>(y, x); 
      frame->operand_stack.push(next_obj_id);
      next_obj_id++;
    }

    else if (instr.opcode() == OpCode::ADDF) {
      VMValue x = frame->operand_stack.top();
      ensure_not_null(*frame, x);
      frame->operand_stack.pop();
      struct_heap[get<int>(x)][get<string>(instr.operand().value())] = nullptr;
    }
    
    else if (instr.opcode() == OpCode::SETF) {
      //value 
      VMValue x = frame->operand_stack.top();
      frame->operand_stack.pop();
      //struct object
      VMValue y = frame->operand_stack.top();
      ensure_not_null(*frame, y);
      frame->operand_stack.pop();
      struct_heap[get<int>(y)][get<string>(instr.operand().value())] = x; 
    }

    else if (instr.opcode() == OpCode::GETF) {
      VMValue x = frame->operand_stack.top();
      ensure_not_null(*frame, x);
      frame->operand_stack.pop();
      frame->operand_stack.push(struct_heap[get<int>(x)][get<string>(instr.operand().value())]);
    }

    else if (instr.opcode() == OpCode::SETI) {
      VMValue x = frame->operand_stack.top();
      frame->operand_stack.pop();
      VMValue y = frame->operand_stack.top();
      frame->operand_stack.pop();
      VMValue z = frame->operand_stack.top();
      ensure_not_null(*frame, z);
      frame->operand_stack.pop();

      if(get<int>(y) >= array_heap[get<int>(z)].size())
        error("out-of-bounds array index", *frame); 

      array_heap[get<int>(z)][get<int>(y)] = x; 
    }
    
    else if (instr.opcode() == OpCode::GETI) {
      VMValue x = frame->operand_stack.top();
      frame->operand_stack.pop();
      VMValue y = frame->operand_stack.top();
      ensure_not_null(*frame, y);
      frame->operand_stack.pop();

      if(get<int>(x) >= array_heap[get<int>(y)].size()){
        error("out-of-bounds array index", *frame);
      }
      else {
        frame->operand_stack.push(array_heap[get<int>(y)][get<int>(x)]);
      }
    }

    else if (instr.opcode() == OpCode::SETI2D) {
      //grab and pop all values from frame op stack
      VMValue value = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      VMValue column = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      VMValue row = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      VMValue id = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 

      //for arr[i][j], the math for 1D is j + i*total columns
      if((get<int>(column)) + (get<int>(row) * col_sizes_2D[get<int>(id)]) >= array_heap[get<int>(id)].size())
        error("out-of-bounds 2D array index" + col_sizes_2D[get<int>(id)], *frame);
      
      //assign value to 1D array heap in frame
      array_heap[get<int>(id)][get<int>(column) + (get<int>(row) * col_sizes_2D[get<int>(id)])] = value;
    }

    else if (instr.opcode() == OpCode::GETI2D) {
      //grab all values from operand stack
      VMValue column = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      VMValue row = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      VMValue id = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 

      //check index in bound
      if(get<int>(column) + (get<int>(row) * col_sizes_2D[get<int>(id)]) >= array_heap[get<int>(id)].size()) 
        error("  out-of-bounds 2D array index" + col_sizes_2D[get<int>(id)], *frame); 
      
      //puhs on the frame op stack the value from arr[row][column]
      frame->operand_stack.push(array_heap[get<int>(id)][get<int>(column) + (get<int>(row) * col_sizes_2D[get<int>(id)])]);
    }

    //for 2d array allocation
    else if (instr.opcode() == OpCode::ALLOCA2D) {
      //grab 
      VMValue x = frame->operand_stack.top(); 
      frame->operand_stack.pop(); 
      int rows = get<int>(frame->operand_stack.top()); 
      frame->operand_stack.pop(); 
      int columns = get<int>(frame->operand_stack.top()); 
      frame->operand_stack.pop(); 

      //allocate a space of rows*columns in stack
      array_heap[next_obj_id] = vector<VMValue>((columns*rows), x); 
      //map of column sizes for a specific 2d array oid
      col_sizes_2D[next_obj_id] = columns; 
      frame->operand_stack.push(next_obj_id); 
      next_obj_id++; 
    }


    //----------------------------------------------------------------------
    // special
    //----------------------------------------------------------------------

    
    else if (instr.opcode() == OpCode::DUP) {
      VMValue x = frame->operand_stack.top();
      frame->operand_stack.pop();
      frame->operand_stack.push(x);
      frame->operand_stack.push(x);      
    }

    else if (instr.opcode() == OpCode::NOP) {
      // do nothing
    }
    
    else {
      error("unsupported operation " + to_string(instr));
    }
  }
}


void VM::ensure_not_null(const VMFrame& f, const VMValue& x) const
{
  if (holds_alternative<nullptr_t>(x))
    error("null reference", f);
}


VMValue VM::add(const VMValue& x, const VMValue& y) const
{
  if (holds_alternative<int>(x)) 
    return get<int>(x) + get<int>(y);
  else
    return get<double>(x) + get<double>(y);
}

// TODO: Finish the rest of the following arithmetic operators

VMValue VM::sub(const VMValue& x, const VMValue& y) const
{
  if (holds_alternative<int>(x))
    return get<int>(x) - get<int>(y);
  else 
    return get<double>(x) - get<double>(y); 
}

VMValue VM::mul(const VMValue& x, const VMValue& y) const
{
  if (holds_alternative<int>(x))
    return get<int>(x) * get<int>(y);
  else 
    return get<double>(x) * get<double>(y); 
}

VMValue VM::div(const VMValue& x, const VMValue& y) const
{
  if (holds_alternative<int>(x))
    return get<int>(x) / get<int>(y);
  else 
    return get<double>(x) / get<double>(y); 
}


VMValue VM::eq(const VMValue& x, const VMValue& y) const
{
  if (holds_alternative<nullptr_t>(x) and not holds_alternative<nullptr_t>(y)) 
    return false;
  else if (not holds_alternative<nullptr_t>(x) and holds_alternative<nullptr_t>(y))
    return false;
  else if (holds_alternative<nullptr_t>(x) and holds_alternative<nullptr_t>(y))
    return true;
  else if (holds_alternative<int>(x)) 
    return get<int>(x) == get<int>(y);
  else if (holds_alternative<double>(x))
    return get<double>(x) == get<double>(y);
  else if (holds_alternative<string>(x))
    return get<string>(x) == get<string>(y);
  else
    return get<bool>(x) == get<bool>(y);
}

// TODO: Finish the rest of the comparison operators

VMValue VM::lt(const VMValue& x, const VMValue& y) const
{
  if (holds_alternative<int>(x)) 
    return get<int>(x) < get<int>(y);
  else if (holds_alternative<double>(x))
    return get<double>(x) < get<double>(y);
  else if (holds_alternative<string>(x))
    return get<string>(x) < get<string>(y);
  else
    return get<bool>(x) < get<bool>(y);
}

VMValue VM::le(const VMValue& x, const VMValue& y) const
{
  if (holds_alternative<int>(x)) 
    return get<int>(x) <= get<int>(y);
  else if (holds_alternative<double>(x))
    return get<double>(x) <= get<double>(y);
  else if (holds_alternative<string>(x))
    return get<string>(x) <= get<string>(y);
  else
    return get<bool>(x) <= get<bool>(y);
}

VMValue VM::gt(const VMValue& x, const VMValue& y) const
{
  if (holds_alternative<int>(x)) 
    return get<int>(x) > get<int>(y);
  else if (holds_alternative<double>(x))
    return get<double>(x) > get<double>(y);
  else if (holds_alternative<string>(x))
    return get<string>(x) > get<string>(y);
  else
    return get<bool>(x) > get<bool>(y);
}

VMValue VM::ge(const VMValue& x, const VMValue& y) const
{
  if (holds_alternative<int>(x)) 
    return get<int>(x) >= get<int>(y);
  else if (holds_alternative<double>(x))
    return get<double>(x) >= get<double>(y);
  else if (holds_alternative<string>(x))
    return get<string>(x) >= get<string>(y);
  else
    return get<bool>(x) >= get<bool>(y);
}

