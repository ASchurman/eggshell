/* 
 * File:   builtinCommands.h
 * Author: Alexander Schurman
 *
 * Created on November 20, 2012
 * 
 * Interface for the built-in commands (cd, pushd, popd)
 */

#ifndef BUILTINCOMMANDS_H
#define BUILTINCOMMANDS_H

#include "process.h"

#define IS_BUILTIN(x) (strcmp(x, "cd")    == 0 || \
                       strcmp(x, "pushd") == 0 || \
                       strcmp(x, "popd")  == 0)

// Executes a built-in command and returns its exit status. The command to
// execute it determined by cmd->argv[0]
int execBuiltin(CMD* cmd);

#endif
