// process.h                                      Stan Eisenstat (11/04/09)
//
// Header file for process.c

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <setjmp.h>
#include "parse.h"

// Execute command list CMDLIST and return status of last command executed
int process (CMD *cmdList);
