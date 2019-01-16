/* 
 * File:   strBuffer.h
 * Author: Alexander Schurman
 *
 * Created on August 30, 2012, significantly modified November 1, 2012
 * 
 * Provides a growable null-terminated string
 */

#ifndef STRBUFFER_H
#define STRBUFFER_H

typedef struct {
    char* str; // The string held in this strBuffer
    unsigned int size; // The size of the malloc'd block pointed to by str
    unsigned int len; // The length of str (not including termination '\0')
} strBuffer;

/* mallocs a strBuffer and returns a pointer to it.
 * Returns NULL upon failure */
strBuffer* mallocStrBuffer();

// frees the given strBuffer
void freeStrBuffer(strBuffer* buf);

/* Appends the char c to the end of the given strBuffer, growing it if
 * necessary. Returns a pointer to the modified strBuffer. */
strBuffer* strBufferAppend(strBuffer* buf, char c);

/* grows the given strBuffer by an internal constant factor and returns
 * a pointer to it. */
strBuffer* strBufferGrow(strBuffer* buf);

// realloc-s the strBuffer to hold just its string with no extra malloc-ed
// space
strBuffer* strBufferShrink(strBuffer* buf);

#endif
