
#include <stdlib.h>
#include <string.h>
#include "argp-shims.h"

#ifdef _WIN32

char * strndup (const char *s, size_t n)
{
  size_t len = strnlen (s, n);
  char *new = (char *) malloc (len + 1);
  if (new == NULL)
    return NULL;
  new[len] = '\0';
  return (char *) memcpy (new, s, len);
}

char *strchrnul(const char *s, int c)
{
  char * pos = strchr(s, c);
  if (!pos)
    return pos + strlen(s);
  return pos;
}

#endif

