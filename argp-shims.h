
#ifndef _ARGP_SHIMS_H
#define _ARGP_SHIMS_H

#include <stddef.h>

#ifdef _WIN32

#define fwrite_unlocked fwrite
#define putc_unlocked putc
#define fputc_unlocked fputc
#define fputs_unlocked fputs

char *strndup (const char *s, size_t n);
char *strchrnul(const char *s, int c);

#endif

#endif

