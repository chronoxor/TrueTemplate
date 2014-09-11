#ifndef __WINMEM_H
#define __WINMEM_H
#include <windows.h>
#include <wchar.h>
#ifndef USE_BC_RTL

#define memcpy(dst,src,n) MyMemcpy(dst,src,n)
#define memset(dst,src,n) MyMemset(dst,src,n)

//extern HANDLE heap;

//inline void initMem(void)
//{
//  heap = GetProcessHeap();
//}

inline void *myAlloc(size_t size)
{
  return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

inline void myFree(void *block)
{
  if ( block != nullptr ) HeapFree(GetProcessHeap(), 0, block);
}

#define malloc(var) myAlloc(var)
#define free(var) myFree(var)

inline void *operator new(size_t size)
{
  size = size ? size : 1;
  return malloc(size);
}

extern void *MyMemcpy(void *dst, const void *src, size_t n);
extern void *MyMemset(void *dst, int ch, size_t n);

inline void *operator new[](size_t size) { return ::operator new(size); }
inline void *operator new(size_t size, void *p) { return p; }
inline void operator delete(void *ptr) { free(ptr); }
inline void operator delete[](void *ptr) { ::operator delete(ptr); }

#endif
#endif
