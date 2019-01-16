/* 
 * File:   parse.c
 * Author: Alexander Schurman (alexander.schurman@gmail.com)
 *
 * Created on October 27, 2012
 * 
 * Provides an implementation of the parse() function described the parse.h
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "parse.h"
#include "getLine.h"
#include "strBuffer.h"

/*******************************************************************************
 ****************************** Redirection ************************************
 ******************************************************************************/

typedef struct redirection
{
    int type; // redirection type (RED_IN, RED_HERE,
              //                   RED_OUT, RED_OUT_C, RED_OUT_APP,
              //                   RED_OUT_APP_C, RED_ERR, RED_ERR_C,
              //                   RED_ERR_APP, or RED_ERR_APP_C)
    
    char* file; // file (or contents of here document) for redirection
} redirection;

redirection* mallocRedirection()
{
    redirection* red = malloc(sizeof(redirection));
    red->type = NONE;
    red->file = NULL;
    return red;
}

// If red isn't a NULL pointer, frees it and its file field
void freeRedirection(redirection* red)
{
    if(red)
    {
        free(red->file);
        free(red);
    }
}

// Steps through line and appends each char (including the final \n) to doc,
// respecting escapes and environment variables. NOTE: line must have a '\n'
// directly before the terminating '\0' or BAD things will happen.
void readHereDocLine(char* line, strBuffer* doc)
{
    assert(line && line[strlen(line-2)] == '\n');

    bool loop = true; // used to end looping from within the switch
                      // where we can't break out
    for(char* c = line; loop && *c != '\0'; c++)
    {
        switch(*c)
        {
            case '$':
                c++; // move past '$'
                strBuffer* var = mallocStrBuffer(); // env var name

                if(*c == '_' || isalpha(*c))
                {
                    strBufferAppend(var, *c);
                }
                else // not a valid environment variable
                {
                    // don't consider possibility of *c being '\0' right after
                    // '$' because line's last character is '\n'

                    strBufferAppend(doc, '$');
                    strBufferAppend(doc, *c);
                    freeStrBuffer(var);
                    break;
                }
                c++; // move past the char following '$'

                for( ; *c == '_' || isalnum(*c); c++)
                {
                    if(*c == '\0')
                    {
                        loop = false;
                        break;
                    }
                    strBufferAppend(var, *c);
                }

                char* value = getenv(var->str);
                if(value)
                {
                    for(char* p = value; *p != '\0'; p++)
                    {
                        strBufferAppend(doc, *p);
                    }
                }

                if(*c != '\0')
                {
                    strBufferAppend(doc, *c);
                }

                freeStrBuffer(var);
                break;

            case '\\':
                c++;

                // if we didn't escape something, append the first '\\'
                if(*c != '$' && *c != '\\')
                {
                    strBufferAppend(doc, '\\');
                }

                // we know that '\\' can't be followed by '\0' because
                // we know the last char in line is '\n', so append
                strBufferAppend(doc, *c);
                break;

            default:
                strBufferAppend(doc, *c);
                break;
        }
    }
}

// Reads tok, which should be the token directly after a RED_HERE, and malloc-s
// a string for red's file field based on tok. Points tok to the token following
// the last one read. Returns true if successful, false if error.
bool readHereDocument(token** tok, redirection** red)
{
    if(tok == NULL || *tok == NULL || (*tok)->type != SIMPLE)
    {
        return false; // missing HERE in <<HERE
    }
    
    strBuffer* doc = mallocStrBuffer(); // holds the growing here document
    
    // Copy there HERE text into a malloc-ed string called here. Add a '\n' to
    // the end of it before the '\0' so we can compare it to lines from getLine
    char* here = malloc(sizeof(char) * (strlen((*tok)->text) + 2));
    strcpy(here, (*tok)->text);
    here[strlen(here) + 1] = '\0';
    here[strlen(here)] = '\n';
    
    *tok = (*tok)->next; // remove the HERE from tok
    
    char* line;
    while((line = getLine(stdin)) != NULL)
    {
        if(line[strlen(line) - 1] != '\n' || strcmp(line, here) == 0)
        {
            free(line);
            break;
        }
        else
        {
            readHereDocLine(line, doc);
            free(line);
        }
    }

    (*red)->file = strdup(doc->str);
    freeStrBuffer(doc);
    free(here);
    return true;
}

#define IS_IN_REDIRECT(x)  ((x) == RED_IN       || (x) == RED_HERE)

#define IS_OUT_REDIRECT(x) ((x) == RED_OUT      || (x) == RED_OUT_C     || \
                            (x) == RED_OUT_APP  || (x) == RED_OUT_APP_C || \
                            (x) == RED_ERR      || (x) == RED_ERR_C     || \
                            (x) == RED_ERR_APP  || (x) == RED_ERR_APP_C)

#define IS_REDIRECT(x)     (IS_IN_REDIRECT(x)   || IS_OUT_REDIRECT(x))

// Checks the beginning of tok for redirection symbols and updates the stdin
// redirection, redIn, and stdout redirection, redOut, based on tok. Updates
// tok to point to the token following the last one relevant to redirection.
// Returns true if completed successfully, false upon error.
bool checkRedirection(token** tok, redirection** redIn, redirection** redOut)
{
    while(*tok != NULL &&
          (IS_IN_REDIRECT((*tok)->type) || IS_OUT_REDIRECT((*tok)->type)))
    {
        if(IS_IN_REDIRECT((*tok)->type))
        {
            if(*redIn) // if there was already input redirection
            {
                return false;
            }
            else
            {
                *redIn = mallocRedirection();
                (*redIn)->type = (*tok)->type;
                
                *tok = (*tok)->next; // remove the < or <<
                
                if(*tok == NULL || (*tok)->type != SIMPLE)
                {
                    return false;
                }
                else if((*redIn)->type == RED_IN)
                {
                    (*redIn)->file = strdup((*tok)->text);
                    *tok = (*tok)->next; // remove the SIMPLE containing
                                         // the redirection's file field
                }
                else if(!readHereDocument(tok, redIn))
                {
                    return false;
                }
            }
        }
        else // stdout redirection
        {
            if(*redOut) // if there was already output redirection
            {
                return false;
            }
            else
            {
                *redOut = mallocRedirection();
                (*redOut)->type = (*tok)->type;
                
                *tok = (*tok)->next; // remove the output redirection symbol
                
                if(*tok == NULL || (*tok)->type != SIMPLE)
                {
                    return false;
                }
                else
                {
                    (*redOut)->file = strdup((*tok)->text);
                    *tok = (*tok)->next; // remove the SIMPLE containing the
                                         // redirection's file field
                }
            }
        }
    }
    
    return true;
}

// Modifies cmd's fromType, fromFile, toType, and toFile to apply the
// redirection info stored in redIn and redOut. A NULL redIn or redOut means no
// redirection.
void applyRedirection(CMD** cmd, redirection* redIn, redirection* redOut)
{
    if(redIn)
    {
        (*cmd)->fromType = redIn->type;
        (*cmd)->fromFile = redIn->file;
    }
    if(redOut)
    {
        (*cmd)->toType = redOut->type;
        (*cmd)->toFile = redOut->file;
    }
}

// Traverses the command tree, checking for multiple redirection caused by a
// pipe in addition to a redirection symbol. Returns true if valid, false if
// there's multiple redirection.
bool checkMultipleRedirection(CMD* root)
{
    if(root == NULL)
    {
        return true;
    }
    
    if(ISPIPE(root->type))
    {
        if(root->left->toType != NONE)
        {
            return false;
        }
        
        if(ISPIPE(root->right->type))
        {
            if(root->right->left->fromType != NONE)
            {
                return false;
            }
        }
        else if(root->right->fromType != NONE)
        {
            return false;
        }
    }
    
    return checkMultipleRedirection(root->left) &&
           checkMultipleRedirection(root->right);
}

/*******************************************************************************
 *************************** parseSomething Functions **************************
 ******************************************************************************/

/* Several functions in this file take the form:
 * token* parseSomething(token* tok, CMD** cmdOut)
 * where Something is Simple, Stage, Pipeline, AndOr, or Command. These
 * functions parse tok as their type (<simple>, <stage>, etc...) and populate
 * the fields of the CMD it malloc-s in cmdOut. They return a pointer to the
 * token following the last token parsed.
 * If tok is invalid, these functions return NULL and put NULL in cmdOut. */

// See comment at the top of the "parseSomething Functions" section. In addition
// to that description, parseSimple takes in info about redirections from before
// the first SIMPLE token of this <simple>, and also adds info about
// redirections in between args.
token* parseSimple(token* tok,
                   CMD** cmdOut,
                   redirection** redIn,
                   redirection** redOut)
{
    if(tok == NULL || tok->type != SIMPLE)
    {
        *cmdOut = NULL;
        return NULL;
    }
    
    CMD* simple = mallocCMD();
    simple->type = SIMPLE;
    
    while(tok != NULL && (tok->type == SIMPLE || IS_REDIRECT(tok->type)))
    {
        if(tok->type == SIMPLE)
        {
            // add arg to simple
            simple->argc++;
            int argc = simple->argc; // shorthand
            simple->argv = realloc(simple->argv, sizeof(char*) * (argc + 1));
            simple->argv[argc] = NULL;
            simple->argv[argc - 1] = strdup(tok->text);
            
            tok = tok->next; // move past the SIMPLE just read
        }
        else if(!checkRedirection(&tok, redIn, redOut))
        {
            // invalid redirection
            freeCMD(simple);
            *cmdOut = NULL;
            return NULL;
        }
    }
    
    *cmdOut = simple;
    return tok;
}

// parseCommand is called by parseStage, so this function prototype is here
token* parseCommand(token* tok, CMD** cmdOut);

// See comment at the top of the "parseSomething Functions" section
token* parseStage(token* tok, CMD** cmdOut)
{
    if(tok == NULL)
    {
        *cmdOut = NULL;
        return NULL;
    }
    
    redirection* redIn = NULL; // stdin redirection info
    redirection* redOut = NULL; // stdout redirection info
    
    if(!checkRedirection(&tok, &redIn, &redOut))
    {
        freeRedirection(redIn);
        freeRedirection(redOut);
        *cmdOut = NULL;
        return NULL;
    }
    
    if(tok->type == PAR_LEFT) // if SUBCMD
    {
        CMD* command = NULL;
        
        tok = tok->next; // remove PAR_LEFT
        tok = parseCommand(tok, &(command));
        if(command == NULL || tok == NULL || tok->type != PAR_RIGHT)
        {
            if(command) freeCMD(command);
            freeRedirection(redIn);
            freeRedirection(redOut);
            *cmdOut = NULL;
            return NULL;
        }
        else
        {
            tok = tok->next; // remove PAR_RIGHT
            
            if(!checkRedirection(&tok, &redIn, &redOut))
            {
                freeRedirection(redIn);
                freeRedirection(redOut);
                *cmdOut = NULL;
                return NULL;
            }
            
            *cmdOut = mallocCMD();
            (*cmdOut)->type = SUBCMD;
            (*cmdOut)->left = command;
            applyRedirection(cmdOut, redIn, redOut);
            if(redIn) free(redIn); // don't free its file field; it's in cmdOut
            if(redOut) free(redOut); // see above
            return tok;
        }
    }
    else // <simple>
    {
        CMD* simple = NULL;
        tok = parseSimple(tok, &simple, &redIn, &redOut);
        
        if(simple == NULL)
        {
            freeRedirection(redIn);
            freeRedirection(redOut);
            *cmdOut = NULL;
            return NULL;
        }
        else
        {            
            applyRedirection(&simple, redIn, redOut);
            if(redIn) free(redIn); // don't free its file field; it's in simple
            if(redOut) free(redOut); // see above
            *cmdOut = simple;
            return tok;
        }
    }
}

// See comment at the top of the "parseSomething Functions" section
token* parsePipeline(token* tok, CMD** cmdOut)
{
    if(tok == NULL)
    {
        *cmdOut = NULL;
        return NULL;
    }
    
    CMD* stage = NULL;
    tok = parseStage(tok, &stage);
    
    // if error in parseStage
    if(stage == NULL)
    {
        return NULL;
    }
    // if there's nothing in the <pipeline> after the <stage>
    else if(tok == NULL || !ISPIPE(tok->type))
    {
        *cmdOut = stage;
        return tok;
    }
    else // the <stage> is followed by || or |&
    {
        CMD* pipelineRoot = mallocCMD();
        pipelineRoot->type = tok->type;
        pipelineRoot->left = stage;
        
        tok = tok->next;
        tok = parsePipeline(tok, &(pipelineRoot->right));
        
        if(pipelineRoot->right == NULL) // if error in parsePipeline
        {
            freeCMD(pipelineRoot);
            *cmdOut = NULL;
            return NULL;
        }
        else
        {
            *cmdOut = pipelineRoot;
            return tok;
        }
    }
}

// See comment at the top of the "parseSomething Functions" section
token* parseAndOr(token* tok, CMD** cmdOut)
{
    if(tok == NULL)
    {
        *cmdOut = NULL;
        return NULL;
    }
    
    CMD* pipeline = NULL;
    tok = parsePipeline(tok, &pipeline);
    
    // if error in parsePipeline
    if(pipeline == NULL)
    {
        return NULL;
    }
    // if there's nothing in the <and-or> after the <pipeline>
    else if(tok == NULL || (tok->type != SEP_AND && tok->type != SEP_OR))
    {
        *cmdOut = pipeline;
        return tok;
    }
    else // the <pipeline> is followed by && or ||
    {
        CMD* andorRoot = mallocCMD();
        andorRoot->type = tok->type;
        andorRoot->left = pipeline;
        
        tok = tok->next;
        tok = parseAndOr(tok, &(andorRoot->right));
        
        if(andorRoot->right == NULL) // if error in parseAndOr
        {
            freeCMD(andorRoot);
            *cmdOut = NULL;
            return NULL;
        }
        else
        {
            *cmdOut = andorRoot;
            return tok;
        }
    }
}

// See comment at the top of the "parseSomething Functions" section
token* parseCommand(token* tok, CMD** cmdOut)
{
    if(tok == NULL)
    {
        *cmdOut = NULL;
        return NULL;
    }
    
    CMD* andor = NULL;
    tok = parseAndOr(tok, &andor);
    
    // if error in parseAndOr
    if(andor == NULL)
    {
        *cmdOut = NULL;
        return NULL;
    }
    // if there's nothing in the command following the <and-or>
    else if(tok == NULL || (tok->type != SEP_END && tok->type != SEP_BG))
    {
        *cmdOut = andor;
        return tok;
    }
    else // the <and-or> is followed by ; or &
    {
        CMD* commandRoot = mallocCMD();
        commandRoot->type = tok->type;
        commandRoot->left = andor;
        
        tok = tok->next;
        // if a command follows the ; or &
        if(tok != NULL && tok->type != PAR_RIGHT)
        {
            tok = parseCommand(tok, &(commandRoot->right));
            if(commandRoot->right == NULL) // if error in parseCommand()
            {
                freeCMD(commandRoot);
                *cmdOut = NULL;
                return NULL;
            }
        }
        
        *cmdOut = commandRoot;
        return tok;
    }
}


CMD* parse(token* tok)
{
    if(tok == NULL)
    {
        return NULL;
    }
    
    CMD* parsed = NULL;
    tok = parseCommand(tok, &parsed);
    
    // if parseCommand didn't parse the entirety of tok, or it found an error
    if(tok != NULL || parsed == NULL || !checkMultipleRedirection(parsed))
    {
        if(parsed) freeCMD(parsed);
        fprintf(stderr, "Error in parsing tokens.\n");
        return NULL;
    }
    else
    {
        return parsed;
    }
}
