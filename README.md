# Eggshell

Eggshell is a simple Linux shell based on a subset of csh's functionality, but
because eggshells are white, this shell only takes the white-space characters
space and tab as input by default. This makes Eggshell totally unusuable by
default, but it sure was fun to make!

It's also possible to compile Eggshell to take normal input as csh does. See
the Compiling section for details.

Since Eggshell is built on a project originally completed for Yale University's
CS 323 course, some of the code was written by Professor Stan Eisenstat.
Comments at the top of files indicate the author.

## Compiling

Use `make` to compile Eggshell. Note that this project adheres to the C99
standard and may not compile under other C standards. Also note that this
project is specific to Linux and will not compile on other operating systems.

Passing `DBG=1` as an argument to `make` compiles in debug mode.

Passing `NORM=1` as an argument to `make` compiles a more typical shell that
is not restricted to white-space input.

## White-Space Input

Unless built with `make NORM=1` as described above, Eggshell accepts only
space and tab characters as input. Any other characters are interpreted as EOF.

Space and tab encode characters in ASCII, with space representing a 1 and tab
a 0. Because all ASCII characters have 0 as their most significant bit, that bit
is left out of the encoding; only 7 white-space characters are used to encode
each ASCII character.

For ease of debugging, I wrote a small Python script toWhitespace.py, which
takes a quoted string as its first (and only) argument and writes to stdout its
white-space encoding as per this specification.

### Example

Let's imagine we want to execute `ls` with Eggshell. Instead of typing `ls` literally, we first consider the ASCII encoding of it:

`l -> 0x6C -> 0110 1100`

`s -> 0x73 -> 0111 0011`

`ls -> 0x6C73 -> 0110 1100 0111 0011`

Now we drop the most significant bit of each character.

`110 1100 111 0011`

Each 1 is encoded as a space and each 0 is encoded as a tab.
