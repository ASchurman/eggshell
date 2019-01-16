/* 
 * File:   main.c
 * Original Author: Stan Eisenstat
 * Modified by: Alexander Schurman (alexander.schurman@gmail.com)
 *
 * Modified on 20 January 2013 (Sunday)
 * 
 * Prompts for commands, parses them into command structures, and executes the
 * commands.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "getLine.h"
#include "parse.h"
#include "process.h"

int main(int argc, char** argv)
{
    int nCmd = 1; // Command number
    char *line;   // Initial command line
    token *list;  // Linked list of tokens
    CMD *cmd;     // Parsed command

    for( ; ; free(line))
    {
        // Prompt for command
        printf("(%d)$ ", nCmd);
        fflush(stdout);

        // Read line
        if((line = getLine(stdin)) == NULL)
	    {
            break; // Break on end of file
        }

        // Lex line into tokens
	    if((list = tokenize(line)) == NULL)
        {
            continue;
        }

	    if ((cmd = parse(list)) != NULL) // Parsed command?
        {
            process(cmd); // Execute command
            freeCMD(cmd); // Free associated storage
            nCmd++;       // Adjust prompt
        }

        freeList(list); // Free token list
    }

    return EXIT_SUCCESS;
}


// Allocate, initialize, and return a pointer to an empty command structure
CMD* mallocCMD()
{
    CMD* new = malloc(sizeof(CMD));

    new->type     = NONE;
    new->argc     = 0;
    new->argv     = malloc(sizeof(char*));
    new->argv[0]  = NULL;
    new->fromType = NONE;
    new->fromFile = NULL;
    new->toType   = NONE;
    new->toFile   = NULL;
    new->left     = NULL;
    new->right    = NULL;

    return new;
}


// Print arguments in command data structure rooted at *c
void dumpArgs(CMD* c)
{
    for(char **q = c->argv; *q; q++)
    {
        printf(",  argv[%d] = %s", q-(c->argv), *q);
    }
}


// Print input/output redirections in command data structure rooted at *c
void dumpRedirect(CMD* c)
{
    if(c->fromType == NONE && c->fromFile == NULL)
    {
	    ;
    }
    else if(c->fromType == RED_IN && c->fromFile != NULL)
    {
	    printf("  <%s", c->fromFile);
    }
    else if(c->fromType == RED_HERE && c->fromFile != NULL)
    {
	    printf("  <HERE");
    }
    else
    {
	    printf("  ILLEGAL INPUT REDIRECTION");
    }

    if (c->toType == NONE && c->toFile == NULL)
    {
	    ;
    }
    else if(c->toType == RED_OUT       && c->toFile != NULL)
    {
        printf("  >%s",   c->toFile);
    }
    else if(c->toType == RED_OUT_C     && c->toFile != NULL)
    {
	    printf("  >!%s",  c->toFile);
    }
    else if(c->toType == RED_OUT_APP   && c->toFile != NULL)
    {
	    printf("  >>%s",  c->toFile);
    }
    else if(c->toType == RED_OUT_APP_C && c->toFile != NULL)
    {
	    printf("  >>!%s", c->toFile);
    }
    else if(c->toType == RED_ERR       && c->toFile != NULL)
    {
	    printf("  >&%s",   c->toFile);
    }
    else if(c->toType == RED_ERR_C     && c->toFile != NULL)
    {
	    printf("  >&!%s",  c->toFile);
    }
    else if(c->toType == RED_ERR_APP   && c->toFile != NULL)
    {
	    printf("  >>&%s",  c->toFile);
    }
    else if(c->toType == RED_ERR_APP_C && c->toFile != NULL)
    {
	    printf("  >>&!%s", c->toFile);
    }
    else
    {
	    printf("  ILLEGAL OUTPUT REDIRECTION");
    }

    if(c->fromType == RED_HERE && c->fromFile != NULL)
    {
	    printf("\n         HERE:  ");
	    for(char *s = c->fromFile; *s; s++)
        {
	        if(*s != '\n')
            {
		        fputc(*s, stdout);
            }
	        else if(s[1])
            {
		        printf("\n         HERE:  ");
            }
	    }
    }
}


// Print command data structure rooted at *c at level LEVEL
void dumpSimple(CMD* c, int level)
{
    printf("level = %d,  argc = %d", level, c->argc);

    if(c->type == SIMPLE)
    {
	    dumpArgs(c);
    }
    else if(c->type == PIPE)
    {
	    printf(",  PIPE");
    }
    else if(c->type == PIPE_ERR)
    {
	    printf(",  PIPE_ERR");
    }
    else if(c->type == SUBCMD)
    {
	    printf(",  SUBCMD");
    }

    dumpRedirect(c);
}


// Print command data structure rooted at *c; return SEP_END or SEP_BG
int dumpType(CMD* c, int level)
{
    int type = SEP_END;

    if(c->argc < 0)
    {
	    printf("  ARGC < 0");
    }
    else if(c->argv == NULL)
    {
	    printf("  ARGV = NULL");
    }
    else if(c->argv[c->argc] != NULL)
    {
	    printf("  ARGV[ARGC] != NULL");
    }

    if(c->type == SIMPLE)
    {
	    dumpSimple(c, level);
	    if(c->left != NULL)
        {
	        printf("  <simple> HAS LEFT CHILD");
        }
	    if(c->right != NULL)
        {
	        printf("  <simple> HAS RIGHT CHILD");
        }
    }
    else if(c->argc > 0 || c->argv == NULL || c->argv[0] != NULL)
    {
	    printf("  INVALID ARGUMENT LIST IN NON-SIMPLE");
    }
    else if(c->type == SUBCMD)
    {
	    dumpSimple(c, level);
	    printf("\nCMD:   ");
	    type = dumpType(c->left, level+1);
	    if (c->right)
        {
	        printf("  SUBCMD INVALID");
        }
	    char sep = (type == SEP_BG) ? '&' : ';';
	    printf("  %c", sep);
	    type = SEP_END;

    }
    else if(c->fromType != NONE ||
            c->fromFile != NULL ||
	        c->toType != NONE   ||
	        c->toFile != NULL)
    {
	    printf("  INVALID I/O REDIRECTION IN NON-SIMPLE NON-SUBCMD");
    }
    else if(ISPIPE(c->type))
    {
	    dumpSimple (c, level);
	    printf("\nCMD:   ");
	    type = dumpType(c->left, level+1);
	    char* pipe = (c->type == PIPE) ? "|" : "|&";
	    printf("  %s\nCMD: | ", pipe);

	    CMD* p;
	    for(p = c->right; ISPIPE (p->type); p = p->right)
        {
	        type = dumpType(p->left, level+1);
	        char *pipe = (p->type == PIPE) ? "|" : "|&";
	        printf("  %s\nCMD: | ", pipe);
	    }
	    type = dumpType(p, level+1);
	    char sep = (type == SEP_BG) ? '&' : ';';
	    printf("  %c", sep);
	    type = SEP_END;
    }
    else if(c->type == SEP_AND)
    {
	    type = dumpType(c->left, level);
	    printf("  &&\nCMD:   ");
	    type = dumpType(c->right, level);
    }
    else if(c->type == SEP_OR)
    {
	    type = dumpType(c->left, level);
    	printf("  ||\nCMD:   ");
	    type = dumpType(c->right, level);
    }
    else if(c->type == SEP_END)
    {
	    type = dumpType(c->left, level);
	    if(c->right)
        {
	        char sep = (type == SEP_BG) ? '&' : ';';
	        printf("  %c\nCMD:   ", sep);
	        type = dumpType(c->right, level);
	    }
    }
    else if(c->type == SEP_BG)
    {
	    dumpType(c->left, level);
	    type = SEP_BG;
	    if (c->right)
        {
	        printf("  &\nCMD:   ");
	        type = dumpType(c->right, level);
	    }
    }
    else
    {
	    printf("  ILLEGAL CMD TYPE");
    }

    return type;
}


// Print command data structure rooted at *c
void dumpCMD(CMD* c, int level)
{
    printf("CMD:   ");
    int type = dumpType(c, level);
    char sep = (type == SEP_BG) ? '&' : ';';
    printf("  %c\n", sep);
}


// Free tree of commands rooted at *c
void freeCMD(CMD* c)
{
    if(!c)
    {
	    return;
    }

    for(char** p = c->argv; *p; p++)
    {
	    free(*p);
    }
    free(c->argv);

    free(c->fromFile);
    free(c->toFile);

    freeCMD(c->left);
    freeCMD(c->right);

    free(c);
}


// Print list of tokens LIST
void dumpList(struct token* list)
{
    struct token* p;

    for(p = list;  p != NULL;  p = p->next) // Walk down linked list
    {
	    printf("%s:%d ", p->text, p->type); //   printing token and type
    }
    putchar('\n'); // Terminate line
}


// Free list of tokens LIST
void freeList(token* list)
{
    token *p, *pnext;
    for(p = list; p; p = pnext)
    {
	    pnext = p->next;  p->next = NULL; // Zap p->next and p->text
	    free(p->text);    p->text = NULL; //   to stop accidental reuse
	    free(p);
    }
}

// Print in in-order command data structure rooted at *C at depth LEVEL
void dumpTree(CMD* c, int level)
{
    if(!c)
    {
	    return;
    }

    dumpTree(c->left, level+1);

    printf("CMD (Depth = %d):  ", level);

    switch(c->type)
    {
        case SIMPLE:
            printf("SIMPLE");
            dumpArgs(c);
            dumpRedirect(c);
            break;

        case SUBCMD:
            printf("SUBCMD");
            dumpRedirect(c);
            break;

        case PIPE:
            printf("PIPE");
            break;

        case PIPE_ERR:
            printf("PIPE_ERR");
            break;

        case SEP_AND:
            printf("SEP_AND");
            break;

        case SEP_OR:
            printf("SEP_OR");;
            break;

        case SEP_END:
            printf("SEP_END");
            break;

        case SEP_BG:
            printf("SEP_BG");
            break;

        default:
            printf("NONE");
            break;
    }
    printf("\n");

    dumpTree(c->right, level+1);
}
