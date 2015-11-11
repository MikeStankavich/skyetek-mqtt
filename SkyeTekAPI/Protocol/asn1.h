#ifndef STASN1_H
#define STASN1_H

/*********************************************************************
 * 
 * An implementation of a subset of ASN.1 BER/DER encoding.
 *
 * Author: Logan Bruns
 *
 ********************************************************************/

#include <stdlib.h>
#include "utils.h"

/* ASN.1 Modes */

#define ST_ASN1_DECODE 0
#define ST_ASN1_ENCODE 1

/* ASN.1 Setup Routines */

typedef void* st_asn1_context;

int st_asn1_allocate_context(st_asn1_context* pContext);

size_t st_asn1_allocate_context_ext(st_asn1_context* pContext,
                                    void* buffer,
                                    size_t bufferSize);

void st_asn1_free_context(st_asn1_context* pContext);

size_t st_asn1_get_context_size();

void st_asn1_init(st_asn1_context context,
                  int mode,
                  unsigned char* data,
                  size_t dataLength);

int st_asn1_finalize(st_asn1_context context);

/* ASN.1 Common Routines */

int st_asn1_start_sequence(st_asn1_context context);
int st_asn1_finish_sequence(st_asn1_context context);
int st_asn1_start_context_specific(st_asn1_context context, uint64 tag);
int st_asn1_finish_context_specific(st_asn1_context context, uint64 tag);

/* ASN.1 Encode Routines */
int st_asn1_write_integer(st_asn1_context context, int64 w);
int st_asn1_write_octet_string(st_asn1_context context,
                               unsigned char* p,
                               size_t length);
int st_asn1_write_boolean(st_asn1_context context, int boolean);
int st_asn1_write_enumerated(st_asn1_context context, int64 w);
int st_asn1_write_object_identifier(st_asn1_context context,
                                    uint32* id, size_t length);

/* ASN.1 Decode Routines */
int st_asn1_read_integer(st_asn1_context context, int64* w);
int st_asn1_read_octet_string(st_asn1_context context,
                              unsigned char* p,
                              size_t* length);
int st_asn1_read_boolean(st_asn1_context context, int* boolean);
int st_asn1_peek(st_asn1_context context);
int st_asn1_read_enumerated(st_asn1_context context, int64* w);
int st_asn1_read_object_identifier(st_asn1_context context,
                                   uint32* id, size_t* length);

/* ASN.1 Tag Types */

#define ST_SEQUENCE           16
#define ST_ENUMERATED         10
#define ST_OBJECT_IDENTIFIER  6
#define ST_OCTET_STRING       4
#define ST_INTEGER            2
#define ST_BOOLEAN            1

/* ASN.1 BER-TLV Routines */

unsigned short
st_asn1_tlv_parse(unsigned char** data, unsigned short* data_length,
                  unsigned char** value, unsigned short* value_length);

int
st_asn1_tlv_find_tag(unsigned short tag,
                     unsigned char* data, unsigned short data_length,
                     unsigned char** value, unsigned short* value_length);

#endif
