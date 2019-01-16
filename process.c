/* 
 * File:   process.c
 * Author: Alexander Schurman (alexander.schurman@gmail.com)
 *
 * Created on November 18, 2012
 * 
 * Implementation for process() as defined in process.h
 */

#define _GNU_SOURCE

#include <assert.h>
#include "process.h"
#include "builtinCommands.h"

// definitions of file descriptors
#define STDIN_FD  (0)
#define STDOUT_FD (1)
#define STDERR_FD (2)

#define GET_STATUS(x) (WIFEXITED(x) ? WEXITSTATUS(x) : 128 + WTERMSIG(x))

#define EXEC_NAME "eggshell"

// Updates the $? environment variable to contain a base ten string for status
void updateStatusVar(int status)
{
    // 10 digits max in 4 byte int, plus a sign if negative, plus '\0'
    char statusStr[12];
    
    snprintf(statusStr, 12, "%d", status);
    setenv("?", statusStr, 1);
}

// Redirects using dup2() based on the given command's redirection
// fields. Returns 0 for success, -1 for failure. errno is set if -1 is returned
int redirect(CMD* cmd)
{
    if(!cmd) return 0;
    
    fflush(stdout);
    fflush(stderr);
    
    int fd;
    
    if(cmd->fromType == RED_IN)
    {
        if((fd = open(cmd->fromFile, O_RDONLY)) < 0)
        {
            perror(EXEC_NAME);
            return -1;
        }
        if(fd != STDIN_FD)
        {
            dup2(fd, STDIN_FD);
            close(fd);
        }
    }
    else if(cmd->fromType == RED_HERE)
    {
        // use a pipe for HERE documents; a child process will write the doc
        // to a pipe, which the parent process will read
        int fd[2];
        int pid;
        if(pipe(fd) || (pid = fork()) < 0)
        {
            perror(EXEC_NAME);
            return -1;
        }
        else if(pid == 0)
        {
            // child
            close(fd[0]);
            if(fd[1] != STDOUT_FD)
            {
                dup2(fd[1], STDOUT_FD);
                close(fd[1]);
            }
            
            for(char* c = cmd->fromFile; *c != '\0'; c++)
            {
                putchar(*c);
            }
            exit(EXIT_SUCCESS);
        }
        else
        {
            // parent
            close(fd[1]);
            if(fd[0] != STDIN_FD)
            {
                dup2(fd[0], STDIN_FD);
                close(fd[0]);
            }
        }
    }
    
    if(cmd->toType != NONE)
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
        
        if((fd = open(cmd->toFile, options, (mode_t)0666)) < 0)
        {
            perror(EXEC_NAME);
            return -1;
        }
        
        dup2(fd, STDOUT_FD);
        if(ISERROR(cmd->toType))
        {
            dup2(fd, STDERR_FD);
        }
        close(fd);
    }
    return 0;
}

// Executes a <simple> redirection. If background == true, the command is
// executed in the background. Returns the <simple>'s status, or 0 if background
// is true and we don't wait for it to die.
int processSimple(CMD* cmd, bool background)
{
    if(IS_BUILTIN(cmd->argv[0]))
    {
        int status = execBuiltin(cmd);
        updateStatusVar(status);
        return status;
    }
    
    int pid;
    if((pid = fork()) < 0)
    {
        // error in forking
        perror(EXEC_NAME);
        return errno;
    }
    else if(pid == 0)
    {
        // child
        if(redirect(cmd) < 0)
        {
            exit(errno);
        }
        execvp(cmd->argv[0], cmd->argv);
        perror(EXEC_NAME);
        exit(EXIT_FAILURE);
    }
    else
    {
        // parent        
        if(background)
        {
            return 0;
        }
        else
        {
            int status;
            signal(SIGINT, SIG_IGN);
            waitpid(pid, &status, 0);
            signal(SIGINT, SIG_DFL);
            
            int exitStatus = GET_STATUS(status);
            updateStatusVar(exitStatus);
            return exitStatus;
        }
    }
}

// Creates a subshell and executes cmd in it. Returns the status of the
// subcommand. If backgrond == true, the subcommand is executed in the
// background. The redirection info in subcmdNode is applied to the subshell if
// it is non-NULL.
int processSubcommand(CMD* cmd, CMD* subcmdNode, bool background)
{
    int pid;
    if((pid = fork()) < 0)
    {
        // error in forking
        perror(EXEC_NAME);
        return errno;
    }
    else if(pid == 0)
    {
        // child
        if(subcmdNode && redirect(subcmdNode) < 0)
        {
            exit(errno);
        }
        exit(process(cmd));
    }
    else
    {
        // parent
        if(background)
        {
            return 0;
        }
        else
        {
            int status;
            signal(SIGINT, SIG_IGN);
            waitpid(pid, &status, 0);
            signal(SIGINT, SIG_DFL);
            
            int exitStatus = GET_STATUS(status);
            updateStatusVar(exitStatus);
            return exitStatus;   
        }
    }
}

// Executes a <stage> rooted with cmd and returns the status of the last command
// executed.
int processStage(CMD* cmd)
{
    assert(cmd);
    assert(cmd->type == SIMPLE || cmd->type == SUBCMD);
    
    if(cmd->type == SIMPLE)
    {
        return processSimple(cmd, false);
    }
    else
    {
        return processSubcommand(cmd->left, cmd, false);
    }
}

// Executes a pipeline and returns the exit status of the pipe. The arg
// pipeRoot is the PIPE or PIPE_ERR command at the root of the pipeline.
// This function draws upon code from Professor Stan Eisenstat at Yale
// University
int execPipe(CMD* pipeRoot)
{
    // count the number of stages in the pipeline
    int numStages = 1;
    for(CMD* cmd = pipeRoot; ISPIPE(cmd->type); cmd = cmd->right, numStages++);
    
    // create table to hold pid and exit status of all stages in the pipe
    struct {
        int pid, status;
    } processTable[numStages];
    
    int fd[2];             // holds file descriptors for the pipe
    int pid, status;       //   the pid and status of a single stage
    int fdIn = STDIN_FD;   //   the read end of the last pipe, or the original
                           //   stdin
    
    CMD* cmd = pipeRoot;
    for(int i = 0; ISPIPE(cmd->type); cmd = cmd->right, i++)
    {
        if(pipe(fd) < 0 || (pid = fork()) < 0)
        {
            perror(EXEC_NAME);
            return errno;
        }
        else if(pid == 0)
        {
            // child
            close(fd[0]);
            
            // redirect stdin to the last pipe read (if there was a last pipe)
            if(fdIn != STDIN_FD)
            {
                dup2(fdIn, STDIN_FD);
                close(fdIn);
            }
            
            bool shouldCloseFD1 = false;
            // redirect stdout to the new pipe write (if it's not stdout)
            if(fd[1] != STDOUT_FD)
            {
                dup2(fd[1], STDOUT_FD);
                shouldCloseFD1 = true;
            }
            
            // if this is a PIPE_ERR, redirect stderr to the new pipe write
            // (if it's not stderr)
            if(cmd->type == PIPE_ERR && fd[1] != STDERR_FD)
            {
                dup2(fd[1], STDERR_FD);
                shouldCloseFD1 = true;
            }
            
            if(shouldCloseFD1) close(fd[1]);
            
            exit(processStage(cmd->left));
        }
        else
        {
            // parent
            processTable[i].pid = pid;
            
            // close the read end of the last pipe if it's not the orig stdin
            if(i > 0)
            {
                close(fdIn);
            }
            
            fdIn = fd[0]; // remember the read end of the new pipe
            close(fd[1]);
        }
    }
    // cmd is now the right child of last PIPE or PIPE_ERR, the last stage of
    // the pipeline
    
    // if the last stage is a built-in command, it should affect the parent
    // shell, so execute it here instead of forking off a process
    if(cmd->type == SIMPLE && IS_BUILTIN(cmd->argv[0]))
    {
        processTable[numStages - 1].pid = -1; // unused pid
        processTable[numStages - 1].status = processSimple(cmd, false);
        close(fdIn);
    }
    else if((pid = fork()) < 0)
    {
        perror(EXEC_NAME);
        return errno;
    }
    else if(pid == 0)
    {
        // child
        if(fdIn != STDIN_FD)
        {
            dup2(fdIn, STDIN_FD);
            close(fdIn);
        }
        exit(processStage(cmd));
    }
    else
    {
        // parent
        processTable[numStages - 1].pid = pid;
        close(fdIn);
    }
    
    // wait for children to die
    signal(SIGINT, SIG_IGN);
    for(int i = 0; i < numStages; )
    {
        pid = wait(&status);
        int j;
        for(j = 0; j < numStages && processTable[j].pid != pid; j++);
        
        // only add to the processTable if the child's pid is in the table;
        // that is, ignore zombies
        if(j < numStages)
        {
            processTable[j].status = status;
            i++;
        }
    }
    signal(SIGINT, SIG_DFL);
    
    for(int i = 0; i < numStages; i++)
    {
        if(GET_STATUS(processTable[i].status) != 0)
        {
            return GET_STATUS(processTable[i].status);
        }
    }
    return 0;
}

// Executes a <pipeline> rooted with cmd and returns the status of the last
// command executed.
int processPipeline(CMD* cmd)
{
    assert(cmd);
    assert(ISPIPE(cmd->type) || cmd->type == SIMPLE || cmd->type == SUBCMD);
    
    if(ISPIPE(cmd->type))
    {
        int pipeStatus = execPipe(cmd);
        updateStatusVar(pipeStatus);
        return pipeStatus;
    }
    else
    {
        return processStage(cmd);
    }
}

// Executes an <and-or> rooted with cmd and returns the status of the last
// command executed.
int processAndOr(CMD* cmd)
{
    assert(cmd);
    assert(cmd->type != SEP_BG && cmd->type != SEP_END);
    
    int lastStatus = 0; // status of last command executed
    
    if(cmd->type == SEP_AND)
    {
        if((lastStatus = processPipeline(cmd->left)) == 0)
        {
            lastStatus = processAndOr(cmd->right);
        }
    }
    else if(cmd->type == SEP_OR)
    {
        if((lastStatus = processPipeline(cmd->left)) != 0)
        {
            lastStatus = processAndOr(cmd->right);
        }
    }
    else
    {
        lastStatus = processPipeline(cmd);
    }
    return lastStatus;
}

int process(CMD* cmd)
{    
    if(!cmd) return 0;
    
    int zombieStatus; // never used after passing to waitpid
    while(waitpid(-1, &zombieStatus, WNOHANG) > 0); // reap zombie processes
    
    int exitStatus;
    
    if(cmd->type == SEP_BG)
    {
        if(cmd->left->type == SIMPLE)
        {
            processSimple(cmd->left, true);
        }
        else
        {
            processSubcommand(cmd->left, NULL, true);
        }
        exitStatus = process(cmd->right); // cmd->right may be NULL
    }
    else if(cmd->type == SEP_END)
    {
        if(!cmd->right)
        {
            exitStatus = processAndOr(cmd->left);
        }
        else
        {
            processAndOr(cmd->left);
            exitStatus = process(cmd->right);
        }
    }
    else
    {
        exitStatus = processAndOr(cmd);
    }

    return exitStatus;
}
