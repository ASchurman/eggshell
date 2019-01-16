/* 
 * File:   getwc.c
 * Author: Alexander Schurman (alexander.schurman@gmail.com)
 *
 * Created on 20 January 2013 (Sunday)
 * 
 * Gets a single ASCII character from whitespace char input. Returns the
 * character if successful, EOF on end of file, or BAD_INPUT if the input is
 * invalid (reaches EOF before parsing a single ASCII char, non-whitespace
 * input)
 */

#ifndef NORMAL_INPUT

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "getwc.h"

#define IS_WHITESPACE(c) (c == ' ' || c == '\t')

int getwc(FILE* fp)
{
    // return value if a character is parsed from whitespace;
    char outchar = 0;

    for(unsigned char i = 0x40; i; i >>= 1)
    {
        int c = getc(fp);
        if(c == EOF || !IS_WHITESPACE(c))
        {
            return EOF;
        }
        else if(c == ' ') // space -> 1, tab -> 0
        {
            outchar |= i;
        }
    }

    // we're only taking in the low-order 7 bits of the ASCII char, so outchar
    // should be nonnegative
    assert(outchar >= 0);

    return outchar;
}

#endif
