/**
 * SerialDeviceFactory.c
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the SerialDeviceFactory.
 */
#include "../SkyeTekAPI.h"
#include "Device.h"
#include "DeviceFactory.h"
#include "SerialDevice.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#ifndef WIN32
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __sun
#include <sys/mkdev.h>
#endif
#include <fcntl.h>
#endif

#if defined(WIN32)
SKYETEK_STATUS 
SerialDeviceFactory_CreateDevice(
  TCHAR              *address, 
  LPSKYETEK_DEVICE  *lpDevice
  )
{
	HANDLE h;
	
	if( lpDevice == NULL )
		return SKYETEK_INVALID_PARAMETER;

	if(_tcsstr(address, _T("COM")) == NULL)
		return SKYETEK_INVALID_PARAMETER;

	h = CreateFile(address, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if( h == INVALID_HANDLE_VALUE )
		return SKYETEK_FAILURE;
	CloseHandle(h);
	*lpDevice = (LPSKYETEK_DEVICE)malloc(sizeof(SKYETEK_DEVICE));
	memset((*lpDevice),0,sizeof(SKYETEK_DEVICE));

  _tcscpy((*lpDevice)->friendly,address);
  _tcscpy((*lpDevice)->address,address);
  _tcscpy((*lpDevice)->type,SKYETEK_SERIAL_DEVICE_TYPE);

	SerialDevice_InitDevice(*lpDevice);
  return SKYETEK_SUCCESS;
}

unsigned int 
SerialDeviceFactory_DiscoverDevices(
  LPSKYETEK_DEVICE  **lpDevices
  )
{
  unsigned int ix, deviceCount = 0;
	TCHAR portName[12];

  if((lpDevices == NULL) || (*lpDevices != NULL))
    return 0;

	/* Allocate for 10 but might not use all */
  *lpDevices = (LPSKYETEK_DEVICE*)malloc(10 * sizeof(LPSKYETEK_DEVICE));
	memset(*lpDevices,0,(10*sizeof(LPSKYETEK_DEVICE)));

	/* Scan for 10 serial ports */
	memset(portName,0,sizeof(TCHAR)*12);
  for(ix = 1; ix < 10; ix++) 
  {
	#ifdef WINCE	
	  _stprintf(portName, _T("COM%d:"),ix);
	#else
	  _stprintf(portName,  _T("COM%d"), ix);
	#endif
		if( SerialDeviceFactory_CreateDevice(portName, &(*lpDevices)[deviceCount]) != SKYETEK_SUCCESS )
			continue;
    deviceCount++;
  }
  
  return deviceCount;
}

#else

SKYETEK_STATUS
SerialDeviceFactory_CreateDevice(
  char              *address,
  LPSKYETEK_DEVICE  *lpDevice
  )
{
	struct stat deviceStat;
	unsigned int major;
	int fd;

	if(stat(address, &deviceStat) == -1)
		return SKYETEK_INVALID_PARAMETER;

	major = major(deviceStat.st_rdev);
	
	switch(major)
	{
#ifdef LINUX
		/* 4 is standard linux serial
		 * 89 is I2C
		 * 153 is S3C2410 SPI
		 * 204 is S3C2410 serial
		 * 188 is USB to serial
		 */
		case 4:
		case 89:
		case 153:
		case 188:
		case 204:
#elif defined(__sun)
    /* 20 is serial */
    case 20:
#else
#error Need to define platform specific major numbers
#endif
			if((fd = open(address, O_NOCTTY | O_RDWR)) == -1)
				return SKYETEK_FAILURE;

			close(fd);
			*lpDevice = (LPSKYETEK_DEVICE)malloc(sizeof(SKYETEK_DEVICE));
			memset((*lpDevice),0,sizeof(SKYETEK_DEVICE));
      			strcpy((*lpDevice)->friendly,address);
      			strcpy((*lpDevice)->address,address);
  			strcpy((*lpDevice)->type,SKYETEK_SERIAL_DEVICE_TYPE);
			SerialDevice_InitDevice(*lpDevice);
			return SKYETEK_SUCCESS;
	}

	return SKYETEK_FAILURE;
}

unsigned int
SerialDeviceFactory_DiscoverDevices(
  LPSKYETEK_DEVICE  **lpDevices
  )
{
	struct stat devStat;
	unsigned char buffer[64];
	unsigned int ix, iy, deviceCount;
#ifdef LINUX
  char* devPrefixes[] = {"/dev/ttyS", "/dev/ttyUSB", "/dev/spi/", "/dev/i2c/", NULL};
  char* devPaths[] = { NULL };
#elif defined(__sun)
  char* devPrefixes[] = { NULL };
  char* devPaths[] = {"/dev/term/a", "/dev/term/b", NULL};
#else
#error No platform dependent paths specified
#endif
  
	LPSKYETEK_DEVICE lpDevice;
	
	deviceCount = 0;

  for(iy = 0; devPrefixes[iy] != NULL; iy++)
  {
    /* Do 2 ports per device type*/
    for(ix = 0; ix < 2; ix++)
    {
      snprintf(buffer, 64, "%s%d", devPrefixes[iy], ix);
	
      if(stat(buffer, &devStat) == -1)
        continue;

      if( SerialDeviceFactory_CreateDevice(buffer, &lpDevice) != SKYETEK_SUCCESS )
        continue;
	
      deviceCount++;
      *lpDevices = (LPSKYETEK_DEVICE*)realloc(*lpDevices, (deviceCount * sizeof(LPSKYETEK_DEVICE)));
      (*lpDevices)[(deviceCount - 1)] = lpDevice;
    }
  }

  for(iy = 0; devPaths[iy] != NULL; iy++)
  {
    if(stat(devPaths[iy], &devStat) == -1)
      continue;

    if( SerialDeviceFactory_CreateDevice(devPaths[iy], &lpDevice) != SKYETEK_SUCCESS )
      continue;
		
    deviceCount++;
    *lpDevices = (LPSKYETEK_DEVICE*)realloc(*lpDevices, (deviceCount * sizeof(LPSKYETEK_DEVICE)));
    (*lpDevices)[(deviceCount - 1)] = lpDevice;
  }

  fflush(stderr);
  
	return deviceCount;
}
#endif

int 
SerialDeviceFactory_FreeDevice(
  LPSKYETEK_DEVICE lpDevice
  )
{
  if( lpDevice == NULL || lpDevice->internal == NULL )
    return 0;
  if( lpDevice->internal == &SerialDeviceImpl )
  {
    if( SerialDevice_Free(lpDevice) )
      return 1;
  }
  return 0;
}

void 
SerialDeviceFactory_FreeDevices(
  LPSKYETEK_DEVICE    *lpDevices,
  unsigned int        count
  )
{
  unsigned int ix = 0;
	if(lpDevices == NULL)
		return;
  for( ix = 0; ix < count; ix++ )
  {
    if( lpDevices[ix] != NULL )
    {
      if( lpDevices[ix]->internal == &SerialDeviceImpl )
      {
        SerialDeviceFactory_FreeDevice(lpDevices[ix]);
        lpDevices[ix] = NULL;
      }
    }
  }
}

DEVICE_FACTORY SerialDeviceFactory = { 
  SKYETEK_SERIAL_DEVICE_TYPE, 
  SerialDeviceFactory_DiscoverDevices,
  SerialDeviceFactory_FreeDevices,
  SerialDeviceFactory_CreateDevice,
  SerialDeviceFactory_FreeDevice
};
