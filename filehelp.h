/*
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __FIN_FILEHELP_H__
#define __FIN_FILEHELP_H__

#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>

static inline
int readline(std::istream& src,  // src stream
             char* line,         // destination str
             const char* skip) { // skip if starts with this
  int i=0; line[0]=0;
  int skl = strlen(skip);
  do{
    line[i]=src.get();
    line[i+1]=0;
    if(line[i]=='\n') break;
    i++;
  } while(!src.eof());
  if (i>=skl && 0==strncmp(line,skip,skl))
    return readline(src,line,skip);
  return !src.eof();
}

static inline
int freadline(FILE* src,          // src
              char* line,         // destination str
              const char* skip) { // skip if starts with this
  int i=0; line[0]=0;
  int c = 0, skl = strlen(skip);
  while (EOF!=(c=fgetc(src))) {
    line[i]=c;
    line[i+1]=0;
    if(line[i]=='\n') break;
    i++;
  }
  if (i>=skl && 0==strncmp(line,skip,skl))
    return freadline(src,line,skip);
  return EOF!=c;
}

#endif

