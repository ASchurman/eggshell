/* 
 * File:   stack.h
 * Author: Alexander Schurman
 *
 * Created on September 22, 2012
 * Modified to hold strings instead of characters on November 20, 2012
 * 
 * Interface for a stack of strings
 */

#ifndef STACK_H
#define STACK_H

typedef struct
{
    char** data; // the data held in the stack
    unsigned int dataSize; // malloc'd size of data
    unsigned int dataLen; // the number of elements in data
} stack;

// returns a malloc'd stack
stack* mallocStack();

// frees a malloc'd stack
void freeStack(stack* stk);

// pushes a newly malloc-d copy of str onto the stack stk
void stackPush(stack* stk, char* str);

// pops a value off of stk and returns it. Returns NULL if the stack is empty.
// The caller is responsible for freeing the returned string.
char* stackPop(stack* stk);

#endif
