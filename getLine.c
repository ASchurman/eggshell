/* 
 * File:   getLine.c
 * Original Author: Stan Eisenstat
 * Modified By: Alexander Schurman (alexander.schurman@gmail.com)
 *
 * Modified on 20 January 2013 (Sunday)
 * 
 * Read a line of text using the file pointer *fp and returns a pointer to a
 * malloc'd null-terminated string that contains the text read, including the
 * newline (if any) that ends the line. If EOF is reached before any characters
 * are read, NULL is returned.
 */

#include <stdlib.h>
#include "getLine.h"
#include "getwc.h"

char* getLine(FILE* fp)
{
    char* line; // Line being read
    int size;   // #chars allocated
    int c, i;

    size = sizeof(double); // Minimum allocation
    line = malloc(size);
    for(i = 0; (c = getwc(fp)) != EOF; )
    {
        if(i == size-1)
        {
	        size *= 2; // Double allocation
	        line = realloc(line, size);
	    }
	    line[i++] = c;
	    if(c == '\n')
        {
	        break;
        }
    }

    // Check for immediate EOF
    if(c == EOF && i == 0) 
    {
	    free (line);
	    return NULL;
    }

    line[i++] = '\0';         // Terminate line
    line = realloc(line, i);  // Trim excess storage

    return line;
}
