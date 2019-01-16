/* 
 * File:   stack.c
 * Author: Alexander Schurman (alexander.schurman@gmail.com)
 *
 * Created on November 20, 2012
 * 
 * Implementation for a stack of strings
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

#define INIT_STACK_SIZE (10)
#define STACK_GROWTH_FACTOR (2)

stack* mallocStack()
{
    stack* st = malloc(sizeof(stack));
    st->dataLen = 0;
    st->dataSize = INIT_STACK_SIZE;
    st->data = malloc(sizeof(char*) * INIT_STACK_SIZE);
    
    return st;
}

void freeStack(stack* stk)
{
    for(unsigned int i = 0; i < stk->dataLen; i++)
    {
        free(stk->data[i]);
    }
    free(stk->data);
    free(stk);
}

void stackPush(stack* stk, char* str)
{
    // if stack is full, grow it
    if(stk->dataLen == stk->dataSize)
    {
        stk->dataSize *= STACK_GROWTH_FACTOR;
        stk->data = realloc(stk->data, sizeof(char*) * stk->dataSize);
    }
    
    stk->dataLen++;
    stk->data[stk->dataLen - 1] = strdup(str);
}

char* stackPop(stack* stk)
{
    if(stk->dataLen > 0)
    {
        stk->dataLen--;
        return stk->data[stk->dataLen];
    }
    else
    {
        return NULL;
    }
}
