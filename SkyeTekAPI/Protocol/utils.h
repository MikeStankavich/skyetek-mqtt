#ifndef STUTILS_H
#define STUTILS_H

#ifndef WIN32
typedef unsigned int uint32;
typedef int int32;
typedef unsigned long long uint64;
typedef long long int64;
#else
typedef unsigned __int32 uint32;
typedef __int32 int32;
typedef unsigned __int64 uint64;
typedef __int64 int64;
#endif

#ifdef WIN32
#define LITERAL64(X) (X##i64)
#else
#define LITERAL64(X) (X##LL)
#endif

#ifdef WIN32
#include <tchar.h>
#endif

void st_ints2bytes(uint32* src, int count, unsigned char* bytes);

void st_bytes2ints(const unsigned char* src,
                   int offset,
                   uint32* dest,
                   int destLength);

void st_longs2bytes(uint64* src, int count, unsigned char* bytes);

void st_bytes2longs(const unsigned char* src,
                    int offset,
                    uint64* dest,
                    int destLength);

uint32 st_roll_left32(uint32 w, int bits);

uint32 st_roll_right32(uint32 w, int bits);

uint64 st_roll_left64(uint64 w, int bits);

uint64 st_roll_right64(uint64 w, int bits);

void st_memcpy(void* s1,  const void*  s2, int n);

void st_memset(void *s, int c, int n);

int st_memcmp(const void *s1, const void* s2, int n);

void* st_alloc(int size);

void st_free(void* ptr);

#ifdef WIN32
TCHAR* st_alloc_error_message(int err);

void st_dealloc_error_message(TCHAR* s);
#endif

#endif
