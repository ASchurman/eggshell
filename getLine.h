/* 
 * File:   getLine.c
 * Author: Stan Eisenstat
 * 
 * Read a line of text using the file pointer *fp and returns a pointer to a
 * malloc'd null-terminated string that contains the text read, including the
 * newline (if any) that ends the line. If EOF is reached before any characters
 * are read, NULL is returned.
 */

#include <stdio.h>

char* getLine(FILE* fp);
