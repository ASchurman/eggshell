/* 
 * File:   builtinCommands.c
 * Author: Alexander Schurman (alexander.schurman@gmail.com)
 *
 * Created on November 20, 2012
 * 
 * Implementation of the built-in commands (cd, pushd, popd)
 */

#include "builtinCommands.h"
#include "stack.h"

// Executes the cd command with the given args. Returns the exit status.
int cd(CMD* cmd)
{
    int argc = cmd->argc;
    char** argv = cmd->argv;
    
    if(argc > 2)
    {
        fprintf(stderr, "cd: Too many arguments\n");
        return 1;
    }
    else if(((argc == 2) ? chdir(argv[1]) : chdir(getenv("HOME"))) < 0)
    {
        perror("cd");
        return errno;
    }
    else
    {
        return 0;
    }
}

// global directory stack to be used by pushd() and popd()
stack* dirStack = NULL;

// frees dirStack if it has been malloc-d
void freeDirStack()
{
    if(dirStack)
    {
        freeStack(dirStack);
    }
}

// Executes the pushd command with the given args. Returns the exit status.
int pushd(CMD* cmd)
{
    if(!dirStack)
    {
        dirStack = mallocStack();
        atexit(freeDirStack);
    }
    
    if(cmd->argc > 2)
    {
        fprintf(stderr, "pushd: Too many arguments\n");
        return 1;
    }
    else if(cmd->argc == 0)
    {
        fprintf(stderr, "pushd: No directory arg given\n");
        return 1;
    }
    
    char* currentDir = malloc(sizeof(char) * (PATH_MAX + 1));
    if(getcwd(currentDir, PATH_MAX + 1) == NULL)
    {
        free(currentDir);
        fprintf(stderr, "pushd: getcwd failed\n");
        return errno;
    }
    else if(chdir(cmd->argv[1]) < 0)
    {
        free(currentDir);
        fprintf(stderr, "pushd: chdir failed\n");
        return errno;
    }
    else
    {
        stackPush(dirStack, currentDir);
        free(currentDir);
        return 0;
    }
}

// Executes the popd command with the given args, which should be empty.
// Returns the exit status.
int popd(CMD* cmd)
{
    if(!dirStack)
    {
        dirStack = mallocStack();
        atexit(freeDirStack);
    }
    
    if(cmd->argc > 1)
    {
        fprintf(stderr, "popd: Too many arguments\n");
        return 1;
    }
    
    char* dir = stackPop(dirStack);
    if(!dir)
    {
        fprintf(stderr, "popd: Empty directory stack\n");
        return 1;
    }
    else if(chdir(dir) < 0)
    {
        fprintf(stderr, "popd: chdir failed\n");
        free(dir);
        return errno;
    }
    else
    {
        free(dir);
        return 0;
    }
}

int execBuiltin(CMD* cmd)
{
    // file descriptors for stderr used for redirect
    int newStderr = -1, oldStderr = -1;
    
    // redirect stderr if necessary
    if(ISERROR(cmd->toType))
    {
        int options = O_WRONLY;
        if(ISAPPEND(cmd->toType))
        {
            options |= O_APPEND;
            if(!getenv("noclobber") || ISCLOBBER(cmd->toType))
            {
                options |= O_CREAT;
            }
        }
        else
        {
            options |= O_CREAT | O_TRUNC;
            if(getenv("noclobber") && !ISCLOBBER(cmd->toType))
            {
                options |= O_EXCL;
            }
        }
        
        if((newStderr = open(cmd->toFile, options, (mode_t)0666)) < 0)
        {
            perror("eggshell");
            return errno;
        }
        
        fflush(stderr);
        oldStderr = dup(2);
        dup2(newStderr, 2);
        close(newStderr);
    }
    
    int status = 0;
    if(strcmp(cmd->argv[0], "cd") == 0)
    {
        status = cd(cmd);
    }
    else if(strcmp(cmd->argv[0], "pushd") == 0)
    {
        status = pushd(cmd);
    }
    else
    {
        status = popd(cmd);
    }
    
    // redirect stderr back to the original stderr
    if(ISERROR(cmd->toType))
    {
        fflush(stderr);
        dup2(oldStderr, 2);
        close(oldStderr);
    }
    
    return status;
}
