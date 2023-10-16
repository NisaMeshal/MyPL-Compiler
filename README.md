# MyPL-Compiler

This is a compiler for the MyPL programming language, the rules of which I had made. This language is very simple and mainly performs arithmetic and it has 2D array capabilities. An example of a program
can be found under `examples/sudoku.mypl` which is a number-fill game played on a 9x9 board. 

How to set up the program:

```
#initial setup, this will take a while
$> cmake .
$> make

$> ./final_project_tests #to run tests for 2D array functionality on every part of the compiler

$> ./mypl <.mypl file> #to run mypl program
```
