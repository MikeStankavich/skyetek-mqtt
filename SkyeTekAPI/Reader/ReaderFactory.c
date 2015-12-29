/**
 * ReaderFactory.c
 * Copyright � 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the ReaderFactory.
 */
#include "../SkyeTekAPI.h"
#include "Reader.h"
#include "ReaderFactory.h"
#include <string.h>
#include <stdlib.h>
#ifndef __APPLE__
#include <malloc.h>
#endif

static LPREADER_FACTORY ReaderFactories[] = {
  &SkyetekReaderFactory
};

#define READERFACTORIES_COUNT sizeof(ReaderFactories)/sizeof(LPREADER_FACTORY)

unsigned int 
ReaderFactory_GetCount(void)
{
  return READERFACTORIES_COUNT; 
}

LPREADER_FACTORY 
ReaderFactory_GetFactory(
  unsigned int    i
  )
{
  if(i > READERFACTORIES_COUNT)
    return NULL;
  else
    return ReaderFactories[i];
}

SKYETEK_STATUS 
CreateReaderImpl(
  LPSKYETEK_DEVICE    device, 
  LPSKYETEK_READER    *reader
  )
{
  LPREADER_FACTORY pReaderFactory;
  unsigned int ix;
  
  for(ix = 0; ix < ReaderFactory_GetCount(); ix++) 
    {
      pReaderFactory = ReaderFactory_GetFactory(ix);
    
      if(pReaderFactory->CreateReader(device, reader) == SKYETEK_SUCCESS)
        return SKYETEK_SUCCESS;
    
    }
  
  return SKYETEK_FAILURE;
}

unsigned int 
DiscoverReadersImpl(
  LPSKYETEK_DEVICE      *devices, 
  unsigned int          deviceCount, 
  LPSKYETEK_READER      **readers
  )
{
  LPREADER_FACTORY pReaderFactory;
  unsigned int ix, readerCount, localCount;
  LPSKYETEK_READER* localReaders;
  
  readerCount = 0;
  
  for(ix = 0; ix < ReaderFactory_GetCount(); ix++) 
  {
    pReaderFactory = ReaderFactory_GetFactory(ix);
    localReaders = NULL;
    localCount = pReaderFactory->DiscoverReaders(devices, deviceCount, &localReaders);
  
    if(localCount > 0) 
    {
      *readers = (LPSKYETEK_READER*)realloc(*readers, (readerCount + localCount) * sizeof(LPSKYETEK_READER));
      memcpy(((*readers) + readerCount), localReaders, localCount*sizeof(LPSKYETEK_READER));
      free(localReaders);
      readerCount += localCount;
    }
  }

  return readerCount;
}

void 
FreeReadersImpl(
  LPSKYETEK_READER    *readers, 
  unsigned int        count
  )
{
  LPREADER_FACTORY pReaderFactory;
  unsigned int ix;
  for(ix = 0; ix < ReaderFactory_GetCount(); ix++) 
  {
    pReaderFactory = ReaderFactory_GetFactory(ix);
    pReaderFactory->FreeReaders(readers,count);
  }
}

void 
FreeReaderImpl(
  LPSKYETEK_READER lpReader
  )
{
  LPREADER_FACTORY pReaderFactory;
  unsigned int ix;
  for(ix = 0; ix < ReaderFactory_GetCount(); ix++) 
  {
    pReaderFactory = ReaderFactory_GetFactory(ix);
    if( pReaderFactory->FreeReader(lpReader) )
      break;
  }
}

