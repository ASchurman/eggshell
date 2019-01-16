/* 
 * File:   getwc.h
 * Author: Alexander Schurman (alexander.schurman@gmail.com)
 *
 * Created on 20 January 2013 (Sunday)
 * 
 * Gets a single ASCII character from whitespace char input. Returns the
 * character if successful, EOF on end of file or bad input.
 * input)
 */

#ifndef GETWCHAR_H
#define GETWCHAR_H

#ifdef NORMAL_INPUT
#define getwc(x) (getc(x))
#else
int getwc(FILE* fp);
#endif

#endif
