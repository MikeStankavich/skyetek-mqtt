/**
 * TagFactory.c
 * Copyright � 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the TagFactory.
 */
#include "../SkyeTekAPI.h"
#include "Tag.h"
#include "TagFactory.h"
#include <string.h>
#include <stdlib.h>
#ifndef __APPLE__
#include <malloc.h>
#endif


/* NOTE: We can add factories for each tag type 
 *       later if there is tag-specific data that
 *       needs to be stored. For now, it's overkill.
 */
SKYETEK_STATUS CreateTagImpl(SKYETEK_TAGTYPE type, LPSKYETEK_ID lpId, LPSKYETEK_TAG* tag)
{
  LPSKYETEK_TAG t;
  unsigned int ix;
  TCHAR *str = NULL;
  
  if( tag == NULL )
    return SKYETEK_INVALID_PARAMETER;

  t = *tag = (LPSKYETEK_TAG)malloc(sizeof(SKYETEK_TAG));
  if( t == NULL )
    return SKYETEK_OUT_OF_MEMORY;

  memset(t,0,sizeof(SKYETEK_TAG));
  t->type = type;
  InitializeTagType(t);

  if( lpId != NULL && lpId->id != NULL && lpId->length > 0 && lpId->length < SKYETEK_MAX_TAG_LENGTH )
  {
    t->id = SkyeTek_AllocateID(lpId->length);
    if( t->id == NULL )
    {
      free(t);
      return SKYETEK_OUT_OF_MEMORY;
    }
    for(ix = 0; ix < lpId->length; ix++)
      t->id->id[ix] = lpId->id[ix];
  }

  str = SkyeTek_GetStringFromID(t->id);
  if( str != NULL )
  {
    _tcscpy(t->friendly, str);
    SkyeTek_FreeString(str);
  }
  else
  {
    _tcscpy(t->friendly, _T("unknown"));
  }

  return SKYETEK_SUCCESS;
}

SKYETEK_STATUS InitializeTagType(LPSKYETEK_TAG tag)
{
  unsigned int ix = 0;

  if( tag == NULL )
    return SKYETEK_INVALID_PARAMETER;

  ix = tag->type >> 8;
  switch(ix)
  {
    case 0x00:
      if( tag->type == ISO_14443A_AUTO_DETECT || tag->type == ISO_MIFARE_ULTRALIGHT )
        tag->internal = &Iso14443ATagImpl;
      else if( tag->type == ISO_14443B_AUTO_DETECT )
        tag->internal = &Iso14443BTagImpl;
      else
        tag->internal = &GenericTagImpl;
      break;
    case 0x02:
      if( tag->type == MIFARE_DESFIRE )
        tag->internal = &DesfireTagImpl;
      else 
        tag->internal = &Iso14443ATagImpl;
      break;
    case 0x03:
      tag->internal = &Iso14443BTagImpl;
      break;
    default:
      tag->internal = &GenericTagImpl;
      break;
  }
  return SKYETEK_SUCCESS;
}

void FreeTagImpl(LPSKYETEK_TAG tag)
{
  if( tag == NULL )
    return;
  if( tag->id != NULL )
    SkyeTek_FreeID(tag->id);
  free(tag);
}

LPSKYETEK_TAG DuplicateTagImpl(LPSKYETEK_TAG tag)
{
  LPSKYETEK_TAG t = NULL;
  TCHAR *str = NULL;
  unsigned int ix = 0;

  if( tag == NULL )
    return NULL;

  t = (LPSKYETEK_TAG)malloc(sizeof(SKYETEK_TAG));
  if( t == NULL )
    return NULL;

  memset(t,0,sizeof(SKYETEK_TAG));
  
  t->type = tag->type;
  t->afi = tag->afi;
  t->rf = tag->rf;
  t->session = tag->session;

  InitializeTagType(t);

  if( tag->id != NULL )
  {
    t->id = SkyeTek_AllocateID(tag->id->length);
    if( t->id == NULL )
    {
      free(t);
      return NULL;
    }
    for(ix = 0; ix < tag->id->length; ix++)
      t->id->id[ix] = tag->id->id[ix];
  }

  str = SkyeTek_GetStringFromID(t->id);
  if( str != NULL )
  {
    _tcscpy(t->friendly, str);
    SkyeTek_FreeString(str);
  }


  return t;
}

