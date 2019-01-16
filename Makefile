#-------------------------------------------------------------------------------

#	Makefile for Eggshell
#
#	Alexander Schurman
#	alexander.schurman@gmail.com

# target executable name
TARGET	:=eggshell

# define DBG=1 in command line for debug
# define NORM=1 in command line for normal (not whitespace-exclusive)
#     shell input

#-------------------------------------------------------------------------------

CC := gcc

# flags------------------------------------
CFLAGSBASE   := -Wall -Werror -pedantic -std=c99

DEBUGFLAGS   := -g3
RELEASEFLAGS := -O3 -DNDEBUG

VALGRIND     := valgrind -q --tool=memcheck --leak-check=yes

ifeq ($(DBG),1)
	CFLAGS   := $(CFLAGSBASE) $(DEBUGFLAGS)
else
	CFLAGS   := $(CFLAGSBASE) $(RELEASEFLAGS)
endif

ifeq ($(NORM),1)
	CFLAGS   := $(CFLAGS) -DNORMAL_INPUT
endif

# building---------------------------------

SOURCES	:=builtinCommands.c getLine.c main.c parse.c process.c stack.c \
          strBuffer.c tokenize.c getwc.c

OBJ	    :=$(SOURCES:.c=.o)

all: $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $^

main.o:            getLine.h parse.h process.h
stack.o:           stack.h
getLine.o:         getLine.h getwc.h
parse.o:           parse.h getLine.h
strBuffer.o:       strBuffer.h
process.o:         process.h parse.h
builtinCommands.o: builtinCommands.h process.h
stack.o:           stack.h
getwc.o:           getwc.h

valgrind: all
	$(VALGRIND) ./$(TARGET)

# cleaning---------------------------------

clean:
	rm -f $(TARGET) *.o
