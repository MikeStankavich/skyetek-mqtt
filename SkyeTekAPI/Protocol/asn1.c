#include "asn1.h"
#
#if defined(WIN32) || defined(WINCE) 
#include <malloc.h>
#define alloca _alloca
#else
#include <alloca.h>
#endif

/*********************************************************************
 * 
 * An implementation of a subset of ASN.1 BER/DER encoding.
 *
 * Author: Logan Bruns
 *
 ********************************************************************/

#define UNIVERSAL        0x00
#define APPLICATION      0x40
#define CONTEXT_SPECIFIC 0x80

#define CONSTRUCTED      0x20
#define PRIMITIVE        0x00

typedef struct {
  unsigned char allocated;
  int mode;
  unsigned char* data;
  int datalength;
  unsigned char* pos;
  int errors;
} st_asn1_struct;

int
st_asn1_allocate_context(st_asn1_context* pContext) {
  void* p;
  int len;

  len = st_asn1_get_context_size();
  if (len < 0)
    return 0;

  p = st_alloc(len);
  if (p == NULL)
    return 0;

  if (st_asn1_allocate_context_ext(pContext, p, len))
    ((unsigned char*)p)[0] = 1; /* Set allocated flag */
  else
    st_free(p);

  return 1;
}

size_t
st_asn1_allocate_context_ext(st_asn1_context* pContext,
                             void* buffer,
                             size_t bufferSize) {
  if (bufferSize < st_asn1_get_context_size())
    return 0;

  /* Clear allocated flag */
  ((unsigned char*)buffer)[0] = 0;

  *pContext = buffer;
  return 1;
}

void
st_asn1_free_context(st_asn1_context* pContext) {
  if ((pContext == NULL) || (*pContext == NULL))
    return;
  
  if (((unsigned char*)*pContext)[0])
    st_free(*pContext);
  *pContext = NULL;
}  

size_t
st_asn1_get_context_size() {
  return sizeof(st_asn1_struct);
}

void
st_asn1_init(st_asn1_context context,
             int mode,
             unsigned char* data,
             size_t dataLength) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;

  ctx->mode = mode;
  ctx->data = data;
  ctx->datalength = dataLength;
  ctx->pos = ctx->data;
  ctx->errors = 0;
}

int
st_asn1_finalize(st_asn1_context context) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  if (ctx->errors)
    return -ctx->errors;
  else
    return ctx->pos - ctx->data;
}

/* ASN.1 Common Routines */

unsigned char
_log_base_256(uint64 w) {
  unsigned char l = 0;
  do {
    l++;
    w >>= 8;
  } while (w > 0);

  return l;
}

int
_log_base_128(uint64 w) {
  int l = 0;
  do {
    l++;
    w >>= 7;
  } while (w > 0);

  return l;
}

static int
_write_base_128(unsigned char** pos, int remaining, uint64 w) {
  unsigned char* p;
  int l;

  l = _log_base_128(w);
  
  if (remaining < l)
    return 0;

  *pos += l;
  p = *pos - 1;

  *p-- = (unsigned char) w;
  w >>= 7;
  l--;
  
  while (l > 0) {
    *p-- = (unsigned char)((w & 0x7f) | 0x80);
    w >>= 7;
    l--;
  }

  return 1;
}

static int
_read_base_128(unsigned char** p, uint64* w, int max) {
  unsigned char b;
  
  for (*w = 0; max > 0; max--) {
    b = *(*p)++;
    *w = (*w << 7) | (b & 0x7f);
    if (!(b & 0x80))
      break;
  }
  if (max == 0)
    return 0;

  return 1;
}

static int
_write_base_256(st_asn1_struct* ctx, uint64 w, unsigned char length) {
  unsigned char* p;
  
  if ((ctx->pos - ctx->data + length) > ctx->datalength)
    return 0;

  ctx->pos += length;
  p = ctx->pos - 1;

  while (length > 0) {
    *p-- = (unsigned char) w;
    w >>= 8;
    length--;
  }

  return 1;
}

static void
_read_base_256(unsigned char** p, uint64* w, unsigned char l) {
  for (*w = 0; l > 0; l--)
    *w = (*w << 8) | *(*p)++;
}

static int
_write(st_asn1_struct* ctx,
       unsigned char constructed,
       unsigned char clazz,
       uint64 tag,
       unsigned char* data,
       uint64 length) {
  if ((ctx->pos - ctx->data + 2 + length) > ctx->datalength)
    return 0;
  
  if (tag < 31)
    *ctx->pos++ = (unsigned char) (clazz | constructed | tag);
  else {
    *ctx->pos++ = clazz | constructed | 0x1f;
    if (!_write_base_128(&ctx->pos,
                         ctx->datalength - (ctx->pos - ctx->data),
                         tag))
      return 0;
  }

  if (length == 0)
    *ctx->pos++ = 0x80;
  else if (length < 0x80)
    *ctx->pos++ = (unsigned char) length;
  else {
    unsigned char log = _log_base_256(length);
    *ctx->pos++ = 0x80 | log;
    if (!_write_base_256(ctx, length, log))
      return 0;
  }

  if ((ctx->pos - ctx->data + length) > ctx->datalength)
    return 0;

  if (length > 0) {
    st_memcpy(ctx->pos, data, (int) length);
    ctx->pos += length;
  }
  
  return 1;
}

static int
_peek(st_asn1_struct* ctx,
      unsigned char* constructed,
      unsigned char* clazz,
      uint64* tag,
      uint64* length) {
  unsigned char* p;
  unsigned char b;
  
  if ((ctx->pos - ctx->data + 2) > ctx->datalength)
    return 0;

  p = ctx->pos;

  b = *p++;
  *clazz = b & 0xc0;
  *constructed = b & 0x20;
  *tag = b & 0x1f;

  if (*tag == 0x1f) {
    if (!_read_base_128(&p, tag, ctx->datalength - (p - ctx->data)))
      return 0;
  }

  if ((p - ctx->data + 1) > ctx->datalength)
    return 0;
  
  b = *p++;
  if (b & 0x80) {
    b &= 0x7f;
    if (b > 0) {
      if (b > (ctx->datalength - (p - ctx->data)))
        return 0;
      _read_base_256(&p, length, b);
    } else
      *length = 0;
  } else
    *length = b & 0x7f;

  return 1;
}

static int
_read(st_asn1_struct* ctx,
      unsigned char constructed,
      unsigned char clazz,
      uint64 tag,
      unsigned char* data,
      size_t* length) {
  unsigned char* p;
  unsigned char b;
  uint64 rtag, rlength;
  
  if ((ctx->pos - ctx->data + 2) > ctx->datalength)
    return 0;

  p = ctx->pos;

  b = *p++;
  rtag = b & 0x1f;

  if ((clazz != (b & 0xc0)) ||
      (constructed != (b & 0x20)))
    return 0;

  if (rtag == 0x1f)
    if (!_read_base_128(&p, &rtag, ctx->datalength - (p - ctx->data)))
      return 0;
  if (tag != rtag)
    return 0;

  b = *p++;
  if (b & 0x80) {
    b &= 0x7f;
    if (b > 0) {
      if (b > (ctx->datalength - (p - ctx->data)))
        return 0;
      _read_base_256(&p, &rlength, b);
    } else
      rlength = 0;
  } else
    rlength = b & 0x7f;

  if (rlength > *length)
    return 0;
  *length = (size_t) rlength;

  if (rlength > 0)
    st_memcpy(data, p, (int) rlength);
  p += rlength;

  ctx->pos = p;

  return 1;
}

static int
_write_end_of_contents(st_asn1_struct* ctx) {
  if ((ctx->pos - ctx->data + 2) > ctx->datalength)
    return 0;

  *ctx->pos++ = 0;
  *ctx->pos++ = 0;

  return 1;
}

static int
_read_end_of_contents(st_asn1_struct* ctx) {
  if ((ctx->pos - ctx->data + 2) > ctx->datalength)
    return 0;

  if ((*ctx->pos++ != 0) || (*ctx->pos++ != 0))
    return 0;
  
  return 1;
}

int
st_asn1_start_sequence(st_asn1_context context) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  int result;

  if (ctx->mode == ST_ASN1_ENCODE)
    result = _write(ctx, CONSTRUCTED, UNIVERSAL, ST_SEQUENCE, NULL, 0);
  else {
    size_t l = 0;
    result = _read(ctx, CONSTRUCTED, UNIVERSAL, ST_SEQUENCE, NULL, &l);
  }

  if (!result)
    ctx->errors++;
  return result;
}

int
st_asn1_finish_sequence(st_asn1_context context) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  int result;

  if (ctx->mode == ST_ASN1_ENCODE)
    result = _write_end_of_contents(ctx);
  else
    result = _read_end_of_contents(ctx);

  if (!result)
    ctx->errors++;
  return result;
}

int
st_asn1_start_context_specific(st_asn1_context context, uint64 tag) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  int result;

  if (ctx->mode == ST_ASN1_ENCODE)
    result = _write(ctx, CONSTRUCTED, CONTEXT_SPECIFIC, tag, NULL, 0);
  else {
    size_t length = 0;
    result = _read(ctx, CONSTRUCTED, CONTEXT_SPECIFIC, tag, NULL, &length);
  }

  if (!result)
    ctx->errors++;
  return result;
}

int
st_asn1_finish_context_specific(st_asn1_context context, uint64 tag) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  int result;

  if (ctx->mode == ST_ASN1_ENCODE)
    result = _write_end_of_contents(ctx);
  else
    result = _read_end_of_contents(ctx);

  if (!result)
    ctx->errors++;
  return result;
}

/* ASN.1 Encode Routines */
int
st_asn1_write_integer(st_asn1_context context, int64 w) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  unsigned char buffer[9];
  unsigned char* p;
  int i, negative;

  if (w < 0) {
    negative = 1;
    w = -w - 1;
  } else
    negative = 0;

  p = &buffer[8];
  do {
    *p-- = (unsigned char) w;
    w >>= 8;
  } while (w);

  p++;
  if (*p & 0x80)
    *--p = 0;
  
  if (negative)
    for (i = &buffer[8] - p; i >= 0; i--)
      p[i] = ~p[i];

  if (!_write(ctx, PRIMITIVE, UNIVERSAL, ST_INTEGER, p, &buffer[9]-p)) {
    ctx->errors++;
    return 0;
  } else
    return 1;
}

int
st_asn1_write_enumerated(st_asn1_context context, int64 w) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  unsigned char buffer[9];
  unsigned char* p;
  int i, negative;

  if (w < 0) {
    negative = 1;
    w = -w - 1;
  } else
    negative = 0;

  p = &buffer[8];
  do {
    *p-- = (unsigned char) w;
    w >>= 8;
  } while (w);

  p++;
  if (*p & 0x80)
    *--p = 0;
  
  if (negative)
    for (i = &buffer[8] - p; i >= 0; i--)
      p[i] = ~p[i];

  if (!_write(ctx, PRIMITIVE, UNIVERSAL, ST_ENUMERATED, p, &buffer[9]-p)) {
    ctx->errors++;
    return 0;
  } else
    return 1;
}

int
st_asn1_write_octet_string(st_asn1_context context,
                           unsigned char* p,
                           size_t length) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;

  if (!_write(ctx, PRIMITIVE, UNIVERSAL, ST_OCTET_STRING, p, length)) {
    ctx->errors++;
    return 0;
  } else
    return 1;
}

int
st_asn1_write_object_identifier(st_asn1_context context,
                                uint32* id,
                                size_t length) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  unsigned char *p, *buf;
  unsigned int i, bufLen;

  if (length < 3) {
    ctx->errors++;
    return 0;
  }    

  bufLen = 4 * length;
  buf = (unsigned char*) alloca(bufLen);
  p = buf;
  *p++ = (unsigned char)(id[0] * 40 + id[1]);
  i = 2;

  do {
    _write_base_128(&p,
                    bufLen - (p - buf),
                    id[i]);
    i++;
  } while (i < length);

  if (!_write(ctx, PRIMITIVE, UNIVERSAL, ST_OBJECT_IDENTIFIER, buf, p-buf)) {
    ctx->errors++;
    return 0;
  } else
    return 1;
}

int
st_asn1_write_boolean(st_asn1_context context, int boolean) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  unsigned char b;
  
  b = boolean ? 0xff : 0x00;

  if (!_write(ctx, PRIMITIVE, UNIVERSAL, ST_BOOLEAN, &b, 1)) {
    ctx->errors++;
    return 0;
  } else
    return 1;
}

/* ASN.1 Decode Routines */
int
st_asn1_read_integer(st_asn1_context context, int64* w) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  unsigned char buffer[9];
  unsigned char* p;
  size_t l;

  l = 9;
  if (!_read(ctx, PRIMITIVE, UNIVERSAL, ST_INTEGER, &buffer[0], &l)) {
    ctx->errors++;
    return 0;
  }

  p = &buffer[0];
  if (*p & 0x80)
    *w = LITERAL64(0xFFFFFFFFFFFFFFFF);
  else
    *w = 0;
  
  while (l-- > 0)
    *w = (*w << 8) | *p++;

  return 1;
}

int
st_asn1_read_enumerated(st_asn1_context context, int64* w) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  unsigned char buffer[9];
  unsigned char* p;
  size_t l;

  l = 9;
  if (!_read(ctx, PRIMITIVE, UNIVERSAL, ST_ENUMERATED, &buffer[0], &l)) {
    ctx->errors++;
    return 0;
  }

  p = &buffer[0];
  if (*p & 0x80)
    *w = LITERAL64(0xFFFFFFFFFFFFFFFF);
  else
    *w = 0;
  
  while (l-- > 0)
    *w = (*w << 8) | *p++;

  return 1;
}

int
st_asn1_read_octet_string(st_asn1_context context,
                          unsigned char* p,
                          size_t* length) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  if (!_read(ctx, PRIMITIVE, UNIVERSAL, ST_OCTET_STRING, p, length)) {
    ctx->errors++;
    return 0;
  } else
    return 1;
}

int
st_asn1_read_object_identifier(st_asn1_context context,
                               uint32* id,
                               size_t* length) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  unsigned char *p, *buf;
  uint64 w;
  unsigned int i;
  size_t l;

  l = 4 * *length;
  buf = (unsigned char*) alloca(4 * *length);
  
  if (!_read(ctx, PRIMITIVE, UNIVERSAL, ST_OBJECT_IDENTIFIER, buf, &l) ||
      (l < 2)) {
    ctx->errors++;
    return 0;
  }

  p = buf;
  i = 0;

  id[i++] = *p / 40;
  id[i++] = *p % 40;
  
  p++;

  while ((i < *length) && ((size_t)(p-buf) < l)) {
    if (!_read_base_128(&p, &w, sizeof(uint32)+1)) {
      ctx->errors++;
      return 0;
    }
    id[i++] = (uint32)w;
  }

  *length = i;
  
  return 1;
}

int
st_asn1_read_boolean(st_asn1_context context, int* boolean) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  unsigned char b;
  size_t len;

  len = 1;
  if (!_read(ctx, PRIMITIVE, UNIVERSAL, ST_BOOLEAN, &b, &len)) {
    ctx->errors++;
    return 0;
  }

  *boolean = b == 0 ? 0 : 1;

  return 1;
}

int
st_asn1_peek(st_asn1_context context) {
  st_asn1_struct* ctx = (st_asn1_struct*) context;
  unsigned char constructed;
  unsigned char clazz;
  uint64 tag;
  uint64 length;

  if (((ctx->pos - ctx->data + 2) > ctx->datalength) ||
      ((ctx->pos[0] == 0) && (ctx->pos[1] == 0)) ||
      !_peek(ctx, &constructed, &clazz, &tag, &length))
    return 0;

  if (clazz == UNIVERSAL)
    return (int) tag;
  else
    return -((int) tag);
}

unsigned short
st_asn1_tlv_parse(unsigned char** data, unsigned short* data_length,
                  unsigned char** value, unsigned short* value_length) {
  unsigned char* p;
  unsigned short tag, length;

  p = *data;
  if ((*p & 0x1f) == 0x1f) {
    /* Two byte tag */
    tag = p[0]<<8 | p[1];
    p += 2;
  } else {
    /* One byte tag */
    tag = p[0];
    p++;
  }

  if (*p & 0x80) {
    /* >= 128 length (or more but this code, unlike general crypto's
       version, only supports up to two bytes. */
    if ((*p & 0x7f) == 1) {
      length = p[1];
      p += 2;
    } else if ((*p & 0x7f) == 2) {
      length = p[1]<<8 | p[2];
      p += 3;
    } else
      return 0;
  } else {
    length = *p;
    p++;
  }

  // sanity check
  if (((p-*data) + length) > *data_length)
    return 0;

  *value = p;
  *value_length = length;

  p += length;
  *data_length -= (p - *data);
  *data = p;

  return tag;
}


static int
_find_tag(unsigned short tag,
          unsigned char** data, unsigned short* data_length,
          unsigned char** value, unsigned short* value_length) {
  unsigned short t;

  while ((t = st_asn1_tlv_parse(data, data_length, value, value_length)) != 0) {
    if (t == tag)
      return 1;
    else if (((t < 0x0100) && (t & 0x20)) ||
             ((t > 0x00ff) && (t & 0x2000)))
      if (st_asn1_tlv_find_tag(tag,
                               *value, *value_length,
                               value, value_length))
        return 1;
  }

  return 0;
}

int
st_asn1_tlv_find_tag(unsigned short tag,
                     unsigned char* data, unsigned short data_length,
                     unsigned char** value, unsigned short* value_length) {
  return _find_tag(tag, &data, &data_length, value, value_length);
}

