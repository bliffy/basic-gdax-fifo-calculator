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

#ifndef _TIMEPARSE_H
#define _TIMEPARSE_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <time.h>
#include <stdio.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STR2TIME_DEFAULT_FORMAT "%Y-%m-%dT%H:%M:%S"

static inline time_t timeparse_str2time(
          const char * timestr,
          const char * format)
{
    if (format == NULL)
        format = STR2TIME_DEFAULT_FORMAT;
    struct tm tm;
    if (strptime(timestr, format, &tm) == NULL)
        return 0;
    return timegm(&tm);
    //return mktime(&tm);
}

static inline size_t time2str(
          const struct tm * in,
          char* out, size_t max) {
    return strftime(out,max,STR2TIME_DEFAULT_FORMAT,in);
}

#ifdef __cplusplus
}
#endif

#endif // _TIMEPARSE_H
