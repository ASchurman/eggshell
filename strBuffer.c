/* 
 * File:   strBuffer.c
 * Author: Alexander Schurman (alexander.schurman@yale.edu)
 *
 * Created on August 30, 2012, significantly modified November 1, 2012
 * 
 * Provides a growable null-terminated string
 */

#include <stdio.h>
#include <stdlib.h>
#include "strBuffer.h"

#define STRBUFFER_INIT_SIZE (10)
#define STRBUFFER_GROWTH_FACTOR (2)

strBuffer* mallocStrBuffer()
{
    strBuffer* buf = malloc(sizeof (strBuffer));

    if (!buf)
    {
        return NULL;
    }
    else
    {
        buf->size = STRBUFFER_INIT_SIZE;
        buf->str = malloc(sizeof (char) * STRBUFFER_INIT_SIZE);
        buf->str[0] = '\0';
        buf->len = 0;
        
        return buf;
    }
}

void freeStrBuffer(strBuffer* buf)
{
    free(buf->str);
    free(buf);
}

strBuffer* strBufferAppend(strBuffer* buf, char c)
{
    if ((buf->len + 1) == buf->size)
    {
        strBufferGrow(buf);
    }

    buf->str[buf->len] = c;
    buf->len++;
    buf->str[buf->len] = '\0';

    return buf;
}

strBuffer* strBufferGrow(strBuffer* buf)
{
    buf->size *= STRBUFFER_GROWTH_FACTOR;
    buf->str = realloc(buf->str, buf->size);

    return buf;
}

strBuffer* strBufferShrink(strBuffer* buf)
{
    buf->size = buf->len + 1;
    buf->str = realloc(buf->str, buf->size);
    
    return buf;
}
