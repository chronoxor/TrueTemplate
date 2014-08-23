////////////////////////////////////////////////////////////////
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.

#include "winmem.h"

void *MyMemcpy(void *dst, const void *src, size_t n)
{
  char *d = (char*)dst;
  char *s = (char*)src;
  for ( size_t i = 0 ; i < n ; i++,d++,s++ )
    *d = *s;
  return dst;
}

void *MyMemset(void *dst, int ch, size_t n)
{
  char *d = (char*)dst;
  for ( size_t i = 0 ; i < n ; i++,d++ )
    *d = (char)ch;
  return dst;
}
