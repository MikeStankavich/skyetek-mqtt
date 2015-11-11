/**
 * USBDeviceFactory.c
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the USBDeviceFactory.
 */
#include "../SkyeTekAPI.h"
#include "Device.h"
#include "DeviceFactory.h"
#include "USBDevice.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#if defined(WIN32)
#ifndef WINCE
#include <setupapi.h>
#include <hidsdi.h>
#endif
#endif

#ifdef LINUX
#define VID 0xAFEF
#define PID 0X0F01
#include <usb.h>
#endif

#ifdef WINCE
#define CLASS_NAME_SZ    TEXT("SkyeTek_Driver")
#define CLIENT_REGKEY_SZ TEXT("Drivers\\USB\\ClientDrivers\\") CLASS_NAME_SZ
#endif



#if defined(LINUX) || defined(WIN32)
static unsigned int g_usbDevCount = 0;

SKYETEK_STATUS 
USBDeviceFactory_CreateDevice(
  TCHAR              *address, 
  LPSKYETEK_DEVICE  *lpDevice
  )
{
	TCHAR fn[128];

	if(lpDevice == NULL)
		return SKYETEK_INVALID_PARAMETER;
	
	if(_tcsstr(address, _T("COM")) != NULL || _tcsstr(address, _T("SPI")) != NULL || _tcsstr(address, _T("I2C")) != NULL)
		return SKYETEK_INVALID_PARAMETER;

	*lpDevice = (LPSKYETEK_DEVICE)malloc(sizeof(SKYETEK_DEVICE));
	memset((*lpDevice),0,sizeof(SKYETEK_DEVICE));
	_tcscpy((*lpDevice)->address, address);
	_tcscpy((*lpDevice)->type,SKYETEK_USB_DEVICE_TYPE);
	memset(fn,0,128*sizeof(TCHAR));
	_stprintf(fn, _T("%s%d"), SKYETEK_USB_DEVICE_TYPE, ++g_usbDevCount); 
	_tcscpy((*lpDevice)->friendly,fn);
	USBDevice_InitDevice(*lpDevice);
	
	return SKYETEK_SUCCESS;
}

int 
USBDeviceFactory_FreeDevice(
  LPSKYETEK_DEVICE lpDevice
  )
{
  if( lpDevice == NULL || lpDevice->internal == NULL )
    return 0;
  if( lpDevice->internal == &USBDeviceImpl )
  {
    if( USBDevice_Free(lpDevice) )
      return 1;
  }
  return 0;
}
#endif

#if defined(WIN32) && !defined(WINCE)
unsigned int 
USBDeviceFactory_DiscoverDevices(
  LPSKYETEK_DEVICE** lpDevices
  )
{	
	GUID hidGuid;
	HDEVINFO deviceInfo;
	BOOL isSuccess = FALSE;
	unsigned long bytes = 0;   
	SP_INTERFACE_DEVICE_DATA deviceData; 
	PSP_INTERFACE_DEVICE_DETAIL_DATA deviceInterfaceData = 0;   
	unsigned int ix, deviceCount = 0;
	size_t size = 0; 
	SKYETEK_STATUS status;

	if((lpDevices == NULL) || (*lpDevices != NULL))
		return 0;

	g_usbDevCount = 0;
	HidD_GetHidGuid (&hidGuid); 
	deviceInfo = SetupDiGetClassDevs ( &hidGuid, 0, 0, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE  )); 
	if ( deviceInfo == INVALID_HANDLE_VALUE ) 
		return 0; 

	*lpDevices = (LPSKYETEK_DEVICE*)malloc(20 * sizeof(LPSKYETEK_DEVICE));
	memset(*lpDevices,0,(20*sizeof(LPSKYETEK_DEVICE)));
	size = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA); 
	for ( ix = 0; ix < 20; ix++ ) 
	{       
		memset( &deviceData, 0, sizeof(SP_INTERFACE_DEVICE_DATA) );   
		deviceData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA); 
		isSuccess = SetupDiEnumDeviceInterfaces ( deviceInfo, 0, &hidGuid, ix, &deviceData);     
    if ( !isSuccess ) 
		{ 
      if ( ERROR_NO_MORE_ITEMS == GetLastError() ) 
        break; 
      else 
        continue; 
    } 
    SetupDiGetInterfaceDeviceDetail( deviceInfo, &deviceData, 0, 0, &bytes, 0);       
    deviceInterfaceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(bytes * sizeof(BYTE));
    if ( !deviceInterfaceData ) { 
      SetupDiDestroyDeviceInfoList( deviceInfo ); 
      return deviceCount; 
    } 
    memset( deviceInterfaceData, 0, bytes ); 
    deviceInterfaceData->cbSize  = size;
    isSuccess = SetupDiGetInterfaceDeviceDetail( deviceInfo, &deviceData, deviceInterfaceData, bytes, &bytes, 0); 
    if ( !isSuccess ) { 
      free(deviceInterfaceData);
      SetupDiDestroyDeviceInfoList( deviceInfo ); 
      return deviceCount; 
    }

		if((_tcsstr(deviceInterfaceData->DevicePath, _T("vid_afef")) != NULL)
			&& (_tcsstr(deviceInterfaceData->DevicePath, _T("pid_0f01")) != NULL))
		{
			/*printf("DevicePath = %s\r\n", deviceInterfaceData->DevicePath);*/
			status = USBDeviceFactory_CreateDevice(deviceInterfaceData->DevicePath, &((*lpDevices)[deviceCount]));
			if( status != SKYETEK_SUCCESS )
			{
				free(deviceInterfaceData);
				continue;
			}
			deviceCount++;
		}

		free(deviceInterfaceData);
  }

	return deviceCount;
}
#endif

#ifdef LINUX
unsigned int 
USBDeviceFactory_DiscoverDevices(
  LPSKYETEK_DEVICE** lpDevices
  )
{
	struct usb_bus *bus;
	struct usb_device *dev;
	unsigned int deviceCount; 
	LPSKYETEK_DEVICE lpDevice;
	
	if((lpDevices == NULL) || (*lpDevices != NULL))
		return 0;

	deviceCount = 0;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	for(bus = usb_busses; bus; bus = bus->next)
	{
		for(dev = bus->devices; dev; dev = dev->next)
		{
			if((dev->descriptor.idVendor == VID) &&
					(dev->descriptor.idProduct == PID))
			{
				/*printf("USB filename: %s\r\n", dev->filename);*/
				if(USBDeviceFactory_CreateDevice(dev->filename, &lpDevice) != SKYETEK_SUCCESS)
					continue;
				
				/*printf("USB CreateDevice succeded\r\n");*/
				deviceCount++;
				*lpDevices = (LPSKYETEK_DEVICE*)realloc(*lpDevices, (deviceCount * sizeof(LPSKYETEK_DEVICE)));
				*lpDevices[(deviceCount - 1)] = lpDevice;
			}
		}
	}

	return deviceCount;
}
#endif

#ifdef WINCE
unsigned int 
USBDeviceFactory_DiscoverDevices(
	LPSKYETEK_DEVICE** lpDevices
)
{
	SKYETEK_STATUS status;
	unsigned int deviceCount; 

	PDEVMGR_DEVICE_INFORMATION pDevInfo;
	HANDLE hSearch;
	
	if((lpDevices == NULL) || (*lpDevices != NULL))
		return 0;

	deviceCount = 0;
	pDevInfo = NULL;
	hSearch = INVALID_HANDLE_VALUE;

	pDevInfo = (PDEVMGR_DEVICE_INFORMATION)malloc(sizeof(DEVMGR_DEVICE_INFORMATION));
	memset(pDevInfo, 0, sizeof(DEVMGR_DEVICE_INFORMATION));
	pDevInfo->dwSize = sizeof(DEVMGR_DEVICE_INFORMATION);
	hSearch = FindFirstDevice(DeviceSearchByDeviceName, L"SKY*", pDevInfo);
	
	if(hSearch == INVALID_HANDLE_VALUE)
		goto done;

	for(hSearch = FindFirstDevice(DeviceSearchByDeviceName, L"SKY*", pDevInfo); hSearch != INVALID_HANDLE_VALUE; FindNextDevice(hSearch, pDevInfo))
	{
		status = USBDeviceFactory_CreateDevice(pDevInfo->szLegacyName, &((*lpDevices)[deviceCount]));
		
		memset(pDevInfo, 0, sizeof(DEVMGR_DEVICE_INFORMATION));
		pDevInfo->dwSize = sizeof(DEVMGR_DEVICE_INFORMATION);

		if(status != SKYETEK_SUCCESS)		
			continue;

		deviceCount++;
	}

done:
	if(hSearch != INVALID_HANDLE_VALUE)
		FindClose(hSearch);
	
	if(pDevInfo)
		free(pDevInfo);

	return deviceCount;
}
#endif

#if defined(LINUX) || defined(WIN32)
void 
USBDeviceFactory_FreeDevices(
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
      if( lpDevices[ix]->internal == &USBDeviceImpl )
      {
        USBDeviceFactory_FreeDevice(lpDevices[ix]);
        lpDevices[ix] = NULL;
      }
    }
  }
}

DEVICE_FACTORY USBDeviceFactory = {
  SKYETEK_USB_DEVICE_TYPE,
  USBDeviceFactory_DiscoverDevices,
  USBDeviceFactory_FreeDevices,
  USBDeviceFactory_CreateDevice,
  USBDeviceFactory_FreeDevice
};
#endif
