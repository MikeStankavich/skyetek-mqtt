/**
 * SerialDevice.h
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 */
#ifndef SKYETEK_SERIAL_DEVICE_H
#define SKYETEK_SERIAL_DEVICE_H

#include "../SkyeTekAPI.h"
#include "../SkyeTekProtocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Sets the serial device communication options.
 * @param device The device
 * @param settings The settings
 * @return Status
 */
SKYETEK_API SKYETEK_STATUS 
SerialDevice_SetOptions(
  LPSKYETEK_DEVICE            device, 
  LPSKYETEK_SERIAL_SETTINGS   lpSettings
  );

/** 
 * Gets the serial device communication options.
 * @param device The device
 * @param settings The settings
 * @return Status
 */
SKYETEK_STATUS 
SerialDevice_GetOptions(
  LPSKYETEK_DEVICE            device, 
  LPSKYETEK_SERIAL_SETTINGS   lpSettings
  );

/**
 * Initializes the device.
 * @param device The device to initialize.
 */
void 
SerialDevice_InitDevice(
  LPSKYETEK_DEVICE    device
  );

/**
 * Frees the device.
 * @param device The device to free.
 */
int 
SerialDevice_Free(
  LPSKYETEK_DEVICE device
  );


static SKYETEK_SERIAL_SETTINGS SerialDiscoverySettings [] = {
  {38400, 8, NONE, ONE},
  {9600, 8, NONE, ONE},
  {19200, 8, NONE, ONE},
  {57600, 8, NONE, ONE},
  {115200, 8, NONE, ONE}
};

#define NUM_SERIAL_DISCOVERY_SETTINGS (sizeof(SerialDiscoverySettings)/sizeof(SKYETEK_SERIAL_SETTINGS))

#ifdef __cplusplus
}
#endif


#endif
