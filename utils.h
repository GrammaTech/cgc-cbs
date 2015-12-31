/*
* Copyright (C) 2014 - Brian Caswell <bmc@lungetech.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#ifndef _UTILS_H
#define _UTILS_H

#include <err.h>

#ifndef UID_MAX
#define UID_MAX       ((~(uid_t)0)-1)
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifdef DEBUG
#define DEBUG_LINE " (at line %d in %s)", __LINE__, __FILE__
#else
#define DEBUG_LINE
#endif

#define VERIFY(func, args...) do { if(func(args) < 0) err(-1, "unable to call " #func DEBUG_LINE); } while (0);
#define VERIFY_EXP(expression) do { if (!(expression)) err(-1, "unable to verify " #expression DEBUG_LINE); } while (0);
#define VERIFY_ASSN(ret, func, args...) do { if( (ret = func(args)) < 0) err(-1, "unable to call " #func DEBUG_LINE); } while (0);

/* sigh.  stdio.h wraps this in an ifdef that is semi-broken, but the symbol does exist in glibc... */
extern int asprintf (char **__restrict __ptr, __const char *__restrict __fmt, ...);

void get_random(char *buf, unsigned int size);
int is_executable(const char *path);
unsigned long str_to_ulong(const char *s);
unsigned short str_to_ushort(const char *s);
void print_filesizes(const int program_count, char **programs);
char * get_prng_seed();
char * set_prng_seed(const unsigned char *buf, const size_t size);
void print_hash(const unsigned char *hash, const size_t hash_len);
void print_source_identifier(const unsigned char *identifier, const size_t identifer_len);

#endif
