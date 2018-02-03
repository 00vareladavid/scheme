# Notes
* all eval functions will mooch of of Y
* all symbol table functions should take to deal with a copy
*TODO run valgrind to find memroy err
 * run on symbols until error occurs
*TODO refactor before addding other features

# Questions
* what doesn't make sense to copy over?

# Specific Notes
* dont use lvals which where read by READ as the whole tree will be freed by the
  end of the REPL

# General Notes
* always use sizeof with malloc
* copy by default
* decide who is responsible for what
 * do you expect this arg to be unchanged?
 * is the return value meant to be only accessed or modified?
* use enumerated unsigned vals as the return value for erro handling
* to make memory easy, just copy everything
 * later you can see what makes sense to copy and what makes sense to destroy

## Memory management
* functions are responsible for their inputs
* reuse the parts you need and free the parts you dont
* hide memory management through the use of utility functions
* make sure you store any relevant values before you delete
 * this will save you from invalid read

## Function structure
* assertions
 * use control flow to prevent superfluous assertions
* accesing/manipulation
* check for error conditions
* allocate/free memory
 * clearly specify who is responsible for what memory
* don't mix static and dynamic pointers
 * ie dynamically allocated strings and strings on the heap

## Error Handling
* after every xalloc, handle the split between 'enough mem' and 'out of mem'
* only build one semantic peiece at a time, if a memory error occurs, you will
  know what to clean up

### If you caught a memory error
* free any state you consume
* return NULL

### Error Types
* lval -> destroy lval
* sym_tab?

## Refactoring
* first set the type system in a MVP
 * then flesh out the type system with code
* remember to balance top-down with bottom-up

# After
* review C build process
* data structures and algorithms
