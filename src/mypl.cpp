//----------------------------------------------------------------------
// FILE: mypl.cpp
// DATE: Spring 2023
// AUTH: Nisa Meshal
// DESC: MyPL program that implements basic i/o functionalities
//----------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <string>
#include <lexer.h>
#include <token.h>  
#include <simple_parser.h>
#include <ast_parser.h>
#include <print_visitor.h>
#include <semantic_checker.h>
#include <vm.h>
#include <code_generator.h>

using namespace std;

void displayOptions();
void print_one(istream* input);
void print_two(istream* input);
void print_first_word(istream* input);
void print_first_line(istream* input);
void lex_mode(istream* input);
void parse_mode(istream* input); 

//argc gives the count of cmd arguments. 
//argv is an array of argument values. argv[0] is the name of the command.
int main(int argc, char* argv[])
{
  string args[argc];
  istream* input = nullptr; 

  //if there are more than two arguments the program should terminate and the help flag is displayed
  if (argc >  3){
    cout << "Too many arguments were passed in" << endl;
    displayOptions();
    return 1; 
  }

  //converts all arguments into strings and omits the ./mypl argument
  if(argc > 1) {
    args[0] = string(argv[1]);
  }

  if (argc == 3) {
    args[1] = string(argv[2]);
  }

  //if a flag and a filename is passed in 
  if (argc == 3) {
    //create an input filestream using the filename at index one.
    input = new ifstream(args[1]);
    //return an error message and list options if the file doesn't open
    if(input->fail()) {
      cout << "ERROR: Unable to open file '" + args[1] + "'\n";
      return 1;
    }

    //print the first character in the file
    if(args[0] == "--lex") {
      cout << "[Lex Mode]\n";
      lex_mode(input);
    } 
    //print the first two characters in the file
    else if (args[0] == "--parse") {
      cout << "[Parse Mode]\n";
      parse_mode(input);
    } 
    //print the first word in the file
    else if (args[0] == "--print") {
      Lexer lexer = Lexer(*input);
      
      try {
        ASTParser parser(lexer);
        Program p = parser.parse();
        PrintVisitor v(cout);
        p.accept(v);
        } catch (MyPLException& ex) {
          cerr << ex.what() << endl;
        }
    } 
    //print the first line of the file
    else if (args[0] == "--check") {
      Lexer lexer = Lexer(*input);
      
      try {
        ASTParser parser(lexer);
        Program p = parser.parse();
        SemanticChecker v;
        p.accept(v);
      } catch (MyPLException& ex) {
        cerr << ex.what() << endl;
      }

    } 
    //print the first two lines of the file. 
    else if (args[0] == "--ir") {
      Lexer lexer = Lexer(*input); 

      try {
        ASTParser parser(lexer);
        Program p = parser.parse();
        SemanticChecker t;
        p.accept(t);
        VM vm;
        CodeGenerator g(vm);
        p.accept(g);
        cout << to_string(vm) << endl;
      } catch (MyPLException& ex) {
        cerr << ex.what() << endl;
      }
    
    }
    //invalid command was passed in, output error message, options, and then terminate
    else {
      cout << "ERROR: Command not recognized\n";
      displayOptions();
      return 1;
    }
    delete(input);
  } else if (argc == 2) {
    //create a standard input stream. 
    input = &cin;

    //print the first character in the stream
    if(args[0] == "--lex") {
      cout << "[Lex Mode]\n";
      lex_mode(input);
    } 
    //print the first two characters in the file
    else if (args[0] == "--parse") {
      cout << "[Parse Mode]\n";
      parse_mode(input);
    } 
    //print the first word in the file
    else if (args[0] == "--print") {
      Lexer lexer = Lexer(*input);
      
      try {
        ASTParser parser(lexer);
        Program p = parser.parse();
        PrintVisitor v(cout);
        p.accept(v);
        } catch (MyPLException& ex) {
        cerr << ex.what() << endl;
      }
    } 
    //print the first line of the file
    else if (args[0] == "--check") {
      Lexer lexer = Lexer(*input);
      
      try {
        ASTParser parser(lexer);
        Program p = parser.parse();
        SemanticChecker v;
        p.accept(v);
      } catch (MyPLException& ex) {
        cerr << ex.what() << endl;
      }
    } 
    //print the first two lines of the file. 
    else if (args[0] == "--ir") {
      cout << "[Intermediate Mode]\n";
      Lexer lexer = Lexer(*input); 

      try {
        ASTParser parser(lexer);
        Program p = parser.parse();
        SemanticChecker t;
        p.accept(t);
        VM vm;
        CodeGenerator g(vm);
        p.accept(g);
        cout << to_string(vm) << endl;
      } catch (MyPLException& ex) {
        cerr << ex.what() << endl;
      }
    }
    else if (args[0] == "--help") {
      displayOptions();
    }
    //invalid command was passed in, output error message, options, and then terminate
    else {
      string command = args[0];

      //if the input doesn't lead with - it's a file
      if(command[0] != '-'){
        input = new ifstream(args[0]);
        //return an error message and list options if the file doesn't open
        if(input->fail()) {
          cout << "ERROR: Unable to open file '" + args[1] + "'\n";
          return 1;
        } //if the file opens, then print entire file content
        else {
          cout << "[Normal Mode]\n";
          Lexer lexer = Lexer(*input); 
          try {
            ASTParser parser(lexer);
            Program p = parser.parse();
            SemanticChecker t;
            p.accept(t);
            VM vm;
            CodeGenerator g(vm);
            p.accept(g);
            vm.run();
          } catch (MyPLException& ex) {
            cerr << ex.what() << endl;
          }

          delete(input);
        }
      } else {
        cout << "ERROR: Command not recognized\n";
        displayOptions();
        return 1;
      }
    }
  } 
  //if only ./mypl is passed in as argument
  else {
    cout << "[Normal Mode]\n";
    Lexer lexer = Lexer(*input); 
    try {
      ASTParser parser(lexer);
      Program p = parser.parse();
      SemanticChecker t;
      p.accept(t);
      VM vm;
      CodeGenerator g(vm);
      p.accept(g);
      vm.run();
    } catch (MyPLException& ex) {
      cerr << ex.what() << endl;
    }
  }
  
  return 0;
}

void displayOptions() {
  cout << "Usage: ./mypl  [option]  [script-file]" << endl;
  cout << "Options:\n";
  cout << " --help    displays usage options\n";
  cout << " --lex     displays token information\n";
  cout << " --parse   checks for syntax errors\n";
  cout << " --print   pretty prints program\n";
  cout << " --check   statically checks program\n";
  cout << " --ir      print intermediate (code) representation\n";
}

//These are funtions that will print out the needed characters for each command.//

//prints first two characters 
void print_two(istream* input) {
  char ch = input->get();
  cout << ch;
  ch = input->get();
  cout << ch << endl;
}

//prints the first word of the stream
void print_first_word(istream* input) {
  char ch = 'a';
  while(!isspace(ch)) {
    ch = input->get();
    cout << ch;
  }
  cout << endl;
}

//prints the first line of the stream
void print_first_line(istream* input) {
  char ch = ' ';
  while(ch != '\n') {
    ch = input->get();
    cout << ch;
  }
  cout << endl;
}

//lexer function
void lex_mode(istream* input) {
  Lexer lexer = Lexer(*input); 

  try {
    Token t = lexer.next_token();
    cout << to_string(t) << endl;
    while(t.type() != TokenType::EOS) {
      cout << to_string(t) << endl;
      t = lexer.next_token();
    }
  } catch (MyPLException& ex) {
    cerr << ex.what() << endl;
  }
}

void parse_mode(istream* input) {
  Lexer lexer = Lexer(*input);

  try 
  {
    SimpleParser parser(lexer);
    parser.parse();
  } catch (MyPLException& ex) {
    cerr << ex.what() << endl;
  }
}