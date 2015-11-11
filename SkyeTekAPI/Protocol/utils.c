#include "utils.h"
#include <string.h>
#include <stdlib.h>

#ifdef WIN32

#include <windows.h>
#include <lmerr.h>
#include <wininet.h>
#include <stdio.h>
#include <tchar.h>

static const TCHAR* s_no_error_message = _T("Unable to obtain error message");

#endif

void st_ints2bytes(uint32* src, int count, unsigned char* bytes) {
  int i, j;

  j = 0;
  for (i=0; i<count; i++) {
    uint32 w = src[i];
    bytes[j+3] = (unsigned char) w;  w >>= 8;
    bytes[j+2] = (unsigned char) w;  w >>= 8;
    bytes[j+1] = (unsigned char) w;  w >>= 8;
    bytes[j]   = (unsigned char) w;  w >>= 8;
    j += 4;
  }
}

void st_bytes2ints(const unsigned char* src,
                   int offset,
                   uint32* dest,
                   int destLength) {
  int i, j;

  j = offset;
    
  for (i=0; i<destLength; i++,j+=4)
    dest[i] =
      ((uint32)src[j]<<24) |
      (((uint32)src[j+1] & 0xff)<<16) |
      (((uint32)src[j+2] & 0xff)<<8) |
      ((uint32)src[j+3] & 0xff);
}

void st_longs2bytes(uint64* src, int count, unsigned char* bytes) {
  int i, j;

  j = 0;
  for (i=0; i<count; i++) {
    uint64 w = src[i];
    bytes[j+7] = (unsigned char) w;  w >>= 8;
    bytes[j+6] = (unsigned char) w;  w >>= 8;
    bytes[j+5] = (unsigned char) w;  w >>= 8;
    bytes[j+4] = (unsigned char) w;  w >>= 8;
    bytes[j+3] = (unsigned char) w;  w >>= 8;
    bytes[j+2] = (unsigned char) w;  w >>= 8;
    bytes[j+1] = (unsigned char) w;  w >>= 8;
    bytes[j]   = (unsigned char) w;  w >>= 8;
    j += 8;
  }
}

void st_bytes2longs(const unsigned char* src,
                    int offset,
                    uint64* dest,
                    int destLength) {
  int i, j;

  j = offset;
    
  for (i=0; i<destLength; i++,j+=8)
    dest[i] =
      ((uint64)src[j]<<56) |
      (((uint64)src[j+1] & 0xff)<<48) |
      (((uint64)src[j+2] & 0xff)<<40) |
      (((uint64)src[j+3] & 0xff)<<32) |
      (((uint64)src[j+4] & 0xff)<<24) |
      (((uint64)src[j+5] & 0xff)<<16) |
      (((uint64)src[j+6] & 0xff)<<8) |
      ((uint64)src[j+7] & 0xff);
}  

uint32 st_roll_left32(uint32 w, int bits) {
  return (w<<bits) | (w>>(32-bits));
}

uint32 st_roll_right32(uint32 w, int bits) {
  return (w>>bits) | (w<<(32-bits));
}

uint64 st_roll_left64(uint64 w, int bits) {
  return (w<<bits) | (w>>(64-bits));
}

uint64 st_roll_right64(uint64 w, int bits) {
  return (w>>bits) | (w<<(64-bits));
}

void st_memcpy(void* s1,  const  void*  s2, int n) {
#ifndef __ARM_ARCH_4T__
  memcpy(s1, s2, n);
#else
  unsigned char* p1 = s1;
  const unsigned char* p2 = s2;
  while (n-- > 0)
    *p1++ = *p2++;
#endif
}

void st_memset(void *s, int c, int n) {
#ifndef __ARM_ARCH_4T__
  memset(s, c, n);
#else
  unsigned char* p = s;
  while (n-- > 0)
    *p++ = c;
#endif
}

int st_memcmp(const void *s1, const void* s2, int n) {
#ifndef __ARM_ARCH_4T__
  return memcmp(s1, s2, n);
#else
  const unsigned char* p1 = s1;
  const unsigned char* p2 = s2;
  unsigned char c = 0;
  while ((n-- > 0) && (c == 0))
    c = *p1++ - *p2++;

  return c;
#endif
}

void* st_alloc(int size) {
#ifndef __ARM_ARCH_4T__
  return malloc(size);
#else
  return 0;
#endif
}

void st_free(void* ptr) {
#ifndef __ARM_ARCH_4T__
  free(ptr);
#else
  return;
#endif
}

#ifdef WIN32

TCHAR*
st_alloc_error_message(int err)
{
  HMODULE hModule = NULL;
	TCHAR *s;

  DWORD dwFormatFlags =
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_IGNORE_INSERTS |
    FORMAT_MESSAGE_FROM_SYSTEM;

  DWORD dwLastError = err;

  if ((dwLastError >= INTERNET_ERROR_BASE) &&
      (dwLastError <= INTERNET_ERROR_LAST)) {
    hModule = GetModuleHandle(_T("wininet.dll"));
    
    if (hModule != NULL)
      dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
  }
  
	if (FormatMessage(dwFormatFlags,
                     hModule,
                     dwLastError,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     (LPTSTR) &s,
                     0,
                     NULL) == 0) {
    _ftprintf(stderr, _T("FormatMessage failed: %d\n"), GetLastError());
    
		s = (TCHAR*) s_no_error_message;
	}

  return s;
}

void
st_dealloc_error_message(TCHAR* s)
{
	if ((s != NULL) && (s != s_no_error_message))
		if (LocalFree(s) != NULL)
      _ftprintf(stderr, _T("LocalFree() failed: %d\n"), GetLastError());
}

#endif
