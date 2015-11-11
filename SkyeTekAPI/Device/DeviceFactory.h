/**
 * DeviceFactory.h
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Discovers and creates devices.
 */
#ifndef DEVICE_FACTORY_H
#define DEVICE_FACTORY_H

#include "../SkyeTekAPI.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * The factory object for devices.
 */
typedef struct DEVICE_FACTORY
{
  /**
   * ID of this factory.
   */
  TCHAR *id;

  /**
   * Performs auto-discovery of devices.  Auto-discovery is optional
   */
  unsigned int 
  (*DiscoverDevices)(
    LPSKYETEK_DEVICE    **lpDevices
    );

  /**
   * Frees devices.
   */
  void 
  (*FreeDevices)(
    LPSKYETEK_DEVICE    *lpDevices,
    unsigned int        count
    );

  /**
   * Creates a device for a specific device address.
   */
  SKYETEK_STATUS 
  (*CreateDevice)(
    TCHAR              *addr, 
    LPSKYETEK_DEVICE  *lpDevice
    );

  /**
   * Frees a device.
   */
  int 
  (*FreeDevice)(
    LPSKYETEK_DEVICE  lpDevice
    );


} DEVICE_FACTORY, *LPDEVICE_FACTORY;

extern DEVICE_FACTORY SerialDeviceFactory;
extern DEVICE_FACTORY USBDeviceFactory;
#if defined(WIN32) && !defined(WINCE)
extern DEVICE_FACTORY SPIDeviceFactory;
#endif

/**
 * Returns the number of registered device factories
 */
unsigned int 
DeviceFactory_GetCount(void);

/**
 * Retrieves the ith DeviceFactory
 */
LPDEVICE_FACTORY 
DeviceFactory_GetFactory(
  unsigned int    i
  );

/**
 * This discovers all of the devices connected to the host that might
 * have readers attached to them.
 * @param devices Pointer to array to popluate.  This function will allocate memory.
 * @return Number of devices found (size of devices array) 
 */
unsigned int 
DiscoverDevicesImpl(
  LPSKYETEK_DEVICE  **lpDevices
  );

/**
 * Frees the devices.
 * @param devices Devices to free
 */
void 
FreeDevicesImpl(
  LPSKYETEK_DEVICE  *lpDevices,
  unsigned int      count
  );

/**
 * Creates a device from the given deviceAddress
 */
SKYETEK_STATUS 
CreateDeviceImpl(
  TCHAR              *addr, 
  LPSKYETEK_DEVICE  *lpDevice
  );

/**
 * Frees the devices.
 * @param devices Devices to free
 */
void 
FreeDeviceImpl(
  LPSKYETEK_DEVICE lpDevice
  );




#ifdef __cplusplus
}
#endif

#endif 



