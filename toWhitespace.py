#!/usr/bin/env python

# Converts a string passed to sys.argv[1] to whitespace as per the Eggshell
# spec.
# Each bit of an ASCII character is converted to a whitespace character printed
# to stdout.
# 1 -> Space, 0 -> Tab
# Because the most significant bit of all ASCII characters is 0, it is omitted
# as per the Eggshell spec.
#
# If passed a non-ASCII character, toWhitespace exits in error without printing
# anything to stdout.

def toWhitespace(st):
    """Convert string to whitespace and return whitespace string"""
    whiteStLst = []
    for c in st:
        mask = 0x40 # 0x40, not 0x80, because we're throwing out msb
        cInt = ord(c)

        # make sure that the character is valid ASCII, not unicode
        if cInt > 127 or cInt < 0:
            sys.stderr.write("Invalid character; must be ASCII\n")
            sys.exit(1)

        while mask > 0:
            if mask & cInt == 0:
                whiteStLst.append("\t") # 0 -> tab
            else:
                whiteStLst.append(" ")  # 1 -> space
            mask >>= 1

    return "".join(whiteStLst)


if __name__ == "__main__":
    import sys
    try:
        sys.stdout.write(toWhitespace(sys.argv[1]))
    except IndexError:
        pass # no string was passed in sys.argv[1]
