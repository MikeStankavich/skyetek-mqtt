/**
 * ReaderFactory.h
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Creates reader objects.
 */
#ifndef READER_FACTORY_H
#define READER_FACTORY_H

#include "../SkyeTekAPI.h"
#include "Reader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct READER_FACTORY {

  /**
   * Performs auto-discovery of readers.  Auto-discovery is optional
   */
  unsigned int 
  (*DiscoverReaders)(
    LPSKYETEK_DEVICE    *devices, 
    unsigned int        deviceCount, 
    LPSKYETEK_READER    **readers
    );

  /**
   * Frees the readers returns by discover readers.
   */
  void 
  (*FreeReaders)(
    LPSKYETEK_READER *readers,
    unsigned int count
    );

  /**
   * Creates a reader with a given device.
   * The device/address string is ReaderFactory specific.
   */
  SKYETEK_STATUS 
  (*CreateReader)(
    LPSKYETEK_DEVICE  device, 
    LPSKYETEK_READER* reader
    );

  /**
   * Frees the reader returned by create reader.
   */
  int 
  (*FreeReader)(
    LPSKYETEK_READER reader
    );


} READER_FACTORY, *LPREADER_FACTORY;

extern READER_FACTORY SkyetekReaderFactory;
extern READER_FACTORY DemoReaderFactory;

/**
 * Returns the number of registered reader factories
 */
unsigned int 
ReaderFactory_GetCount(void);

/**
 * Retrieves the ith ReaderFactory
 */
LPREADER_FACTORY 
ReaderFactory_GetFactory(
  unsigned int i
  );

/**
 * This discovers all of the readers to be found on the devices that
 * are passed in.
 * @param devices Point to an array of devices
 * @param deviceCount Count of devices in the array
 * @param readers Pointer to array to popluate.  This function will allocate memory.
 * @return Number of readers found (size of readers array) 
 */
unsigned int 
DiscoverReadersImpl(
  LPSKYETEK_DEVICE    *devices, 
  unsigned int        deviceCount, 
  LPSKYETEK_READER    **readers
  );

/**
 * Frees the readers.
 * @param readers Readers to free
 */
void FreeReadersImpl(
  LPSKYETEK_READER    *readers,
  unsigned int        count
  );

/**
 * Creates a reader from the given device.
 */
SKYETEK_STATUS CreateReaderImpl(
  LPSKYETEK_DEVICE    device, 
  LPSKYETEK_READER    *reader
  );

/**
 * Frees a reader.
 */
void FreeReaderImpl(
  LPSKYETEK_READER    reader
  );

#ifdef __cplusplus
}
#endif

#endif 



