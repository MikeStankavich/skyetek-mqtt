/**
 * SPIDevice.h
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 */
#ifndef SPI_DEVICE_H
#define SPI_DEVICE_H

#include "../SkyeTekAPI.h"
#include "../SkyeTekProtocol.h"
#include "../Drivers/aardvark.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_MSG_SIZE 2100

typedef struct SPI_INFO
{
  aa_u16 port_number;
  Aardvark spiHandle;
  int type;
  MUTEX(lock);
} SPI_INFO, *LPSPI_INFO;

/**
 * Initializes the newly created SPI device by 
 * setting up its function table.
 * @param device The device
 */
void SPIDevice_InitDevice(
  LPSKYETEK_DEVICE device
  );

/**
 * Frees any resources used internally by the device
 * @param device The device to destroy
 */
int 
SPIDevice_Free(
  LPSKYETEK_DEVICE    device
  );


#ifdef __cplusplus
}
#endif


#endif
