/**
 * `kyeTekReaderFactory.c
 * Copyright � 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the SkyeTekReaderFactory.
 */
#include "../SkyeTekAPI.h"
#include "Reader.h"
#include "ReaderFactory.h"
#include "../Device/Device.h"
#include "../Protocol/Protocol.h"
#include "../Protocol/STPv2.h"
#include "../Protocol/STPv3.h"
#include "../Device/SerialDevice.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

SKYETEK_STATUS 
STR_GetSystemAddrForParm(
  SKYETEK_SYSTEM_PARAMETER  parameter,
  LPSKYETEK_ADDRESS         lpAddr,
  unsigned int              version
  );

void 
SkyeTek_Debug(
  TCHAR * sz, 
  ...
  );


int 
GetReaderVersion(
  LPSKYETEK_DEVICE    lpDevice
  )
{
  LPDEVICEIMPL lpDI = NULL;
  LPSKYETEK_DATA lpData = NULL;
  TCHAR *msg = NULL;
  int bytes = 0, len = 0;

  if( lpDevice == NULL || lpDevice->internal == NULL )
    return 0;

  lpDI = (LPDEVICEIMPL)lpDevice->internal;

  lpData = SkyeTek_AllocateData(64);
  lpData->data[0] = 0x02;
  lpData->data[1] = 0x00;
  lpData->data[2] = 0x01;

  bytes = lpDI->Write(lpDevice,lpData->data,3,500);
  if( bytes != 3 )
  {
    SkyeTek_FreeData(lpData);
    return 0;
  }
  lpDI->Flush(lpDevice);
  SkyeTek_Debug(_T("Sent: 020001\r\n"));

  SKYETEK_Sleep(100);
  bytes = lpDI->Read(lpDevice,lpData->data,3,500);

  lpData->size = bytes;
  msg = SkyeTek_GetStringFromData(lpData);
  if( msg != NULL )
  {
    SkyeTek_Debug(_T("Read: %s\r\n"), msg);
    SkyeTek_FreeString(msg);
    msg = NULL;
  }
  
  // not a reader
  if( bytes == 0 )
  {
    SkyeTek_FreeData(lpData);
    return 0;
  }

  // bootloader mode
  if( bytes == 2 && lpData->data[1] == 0xFF )
  {
    SkyeTek_FreeData(lpData);
    return -1;
  }
  else if( bytes == 3 && lpData->data[0] == 0x00 && lpData->data[1] == 0x03 )
  {
    SkyeTek_FreeData(lpData);
    return -1;
  }

  // something else?
  if( bytes != 3 )
  {
    SkyeTek_FreeData(lpData);
    return 0;
  }
  
  /* Version 2 */
  if( lpData->data[0] == 0x02 && lpData->data[1] == 0x01 && lpData->data[2] == 0x82 )
  {
    SkyeTek_FreeData(lpData);
    return 2;
  }
  if( lpData->data[0] == 0x02 && lpData->data[1] == 0x01 && lpData->data[2] == 0x81 )
  {
    SkyeTek_FreeData(lpData);
    return 2;
  }
  if( lpData->data[0] == 0x02 && lpData->data[1] == 0x04 && lpData->data[2] == 0x88 )
  {
    bytes = lpDI->Read(lpDevice,lpData->data+3,3,500);
    lpData->size = 3 + bytes;
    SkyeTek_FreeData(lpData);
    return 2;
  }
  if( lpData->data[0] == 0x02 && lpData->data[1] == 0x03 && lpData->data[2] == 0x88 )
  {
    bytes = lpDI->Read(lpDevice,lpData->data+3,2,500);
    lpData->size = 3 + bytes;
    SkyeTek_FreeData(lpData);
    return 2;
  }

  /* Sanity check */
  if( lpData->data[0] != 0x02 && lpData->data[1] != 0x00 )
  {
    SkyeTek_FreeData(lpData);
    return 0;
  }

  /* Get length */
  len = lpData->data[2];
  if( len > 60 || len == 0 )
  {
    /* Garbage */
    SkyeTek_FreeData(lpData);
    return 0;
  }

  /* Assume Version 3 and read remaining bytes */
  bytes = lpDI->Read(lpDevice,lpData->data+3,len,500);
  lpData->size = 3+ bytes;

  msg = SkyeTek_GetStringFromData(lpData);
  if( msg != NULL )
  {
    SkyeTek_Debug(_T("Read: %s\r\n"), msg);
    SkyeTek_FreeString(msg);
    msg = NULL;
  }

  if( bytes != len )
  {
    SkyeTek_FreeData(lpData);
    return 0;
  }

  /* Version 3 */
  if( lpData->data[3] == 0x80 || lpData->data[3] == 0x90 )
  {
    SkyeTek_FreeData(lpData);
    return 3;
  }
  SkyeTek_FreeData(lpData);
  return 0;
}

LPSKYETEK_READER 
GetReader(
  LPSKYETEK_DEVICE    lpDevice, 
  LPPROTOCOLIMPL      lpPI,
  unsigned int        ver
  )
{
  LPSKYETEK_READER lpReader = NULL;
  SKYETEK_READER tmpReader;
  SKYETEK_STATUS status;
  SKYETEK_ADDRESS addr;
  LPSKYETEK_DATA lpData = NULL;
  TCHAR *str = NULL;
  int i = 0;
  unsigned int ix = 0;

  if( lpDevice == NULL )
    return NULL;

  /* Fill temp reader for system calls */
  if( ver == 2 )
  {
    tmpReader.id = SkyeTek_AllocateID(1);
    if( tmpReader.id != NULL )
      tmpReader.id->id[0] = 0xFF;
  }
  else
  {
    tmpReader.id = SkyeTek_AllocateID(4);
    if( tmpReader.id != NULL )
      for( i = 0; i < 4; i++ )
        tmpReader.id->id[i] = 0xFF;
  }
  tmpReader.sendRID = 1;
  tmpReader.lpDevice = lpDevice;
  tmpReader.internal = &SkyetekReaderImpl;

  status = STR_GetSystemAddrForParm(SYS_FIRMWARE,&addr,ver);
  if( status != SKYETEK_SUCCESS )
    goto failure;
  status = lpPI->GetSystemParameter(&tmpReader, &addr, &lpData,100);
  if( status != SKYETEK_SUCCESS || lpData == NULL )
    goto failure;

  lpReader = (LPSKYETEK_READER)malloc(sizeof(SKYETEK_READER));
  if( lpReader == NULL )
  {
    SkyeTek_FreeData(lpData);
    goto failure;
  }
  memset(lpReader, 0, sizeof(SKYETEK_READER));

  str = SkyeTek_GetStringFromData(lpData);
  if( str != NULL )
  {
    _tcscpy(lpReader->firmware,str);
    SkyeTek_FreeString(str);
  }

  if( ver == 2 && lpData != NULL && lpData->data != NULL && lpData->size > 0 )
  {
    switch(lpData->data[0])
    {
      case 0x00:
      case 0x01:
      case 0x50:
      case 0x60:
      case 0xA0:
      case 0xC0:
      case 0xD0:
        _tcscpy(lpReader->model, _T("M1")); 
        break;
      case 0xE0:
        _tcscpy(lpReader->model, _T("M8")); 
        break;
		  case 0x20:
        _tcscpy(lpReader->model, _T("M2")); 
        break;
		  default:
        _tcscpy(lpReader->model, _T("??")); 
    }
  }
  SkyeTek_FreeData(lpData);

  if( ver == 3 )
  {
    SKYETEK_Sleep(100);
    status = STR_GetSystemAddrForParm(SYS_PRODUCT,&addr,ver);
    if( status != SKYETEK_SUCCESS )
      goto failure;
    status = lpPI->GetSystemParameter(&tmpReader, &addr, &lpData,100);
    if( status == SKYETEK_SUCCESS && lpData->data != NULL && lpData->size >= 2 )
    {
      switch(lpData->data[1])
      {
      case 0x02:
        _tcscpy(lpReader->model, _T("M2"));
        break;
      case 0x04:
        _tcscpy(lpReader->model, _T("M4"));
        break;
      case 0x2A:
        _tcscpy(lpReader->model, _T("M2A"));
        break;
      case 0x07:
        _tcscpy(lpReader->model, _T("M7"));
        break;
      case 0x09:
        _tcscpy(lpReader->model, _T("M9"));
        break;
	  case 0x0A:
		_tcscpy(lpReader->model, _T("M10"));
		break;
      default:
        _tcscpy(lpReader->model, _T("??"));
        break;
      }
      SkyeTek_FreeData(lpData);
    }
    else
      _tcscpy(lpReader->model, _T("??"));
  }

  SKYETEK_Sleep(100);
  status = STR_GetSystemAddrForParm(SYS_READER_NAME,&addr,ver);
  if( status != SKYETEK_SUCCESS )
    goto failure;
  status = lpPI->GetSystemParameter(&tmpReader, &addr, &lpData,100);
  if( status != SKYETEK_SUCCESS )
  {
    free(lpReader);
    goto failure;
  }
  if( lpData != NULL && lpData->size > 0 && lpData->data != NULL )
  {
    /* make sure we have a null pointer */
    i = 0;
    for( ix = 0; ix < lpData->size; ix++ )
    {
      if( lpData->data[ix] == '\0' )
      {
        i = 1;
        break;
      }
    }
    if( i == 1 )
	{
		//Reader name is stored in standard ascii
		for(ix = 0; ix < lpData->size; ix++)
			lpReader->readerName[ix] = (TCHAR)lpData->data[ix];
	}
  }
  SkyeTek_FreeData(lpData);

  SKYETEK_Sleep(100);
  status = STR_GetSystemAddrForParm(SYS_SERIALNUMBER,&addr,ver);
  if( status != SKYETEK_SUCCESS )
    goto failure;
  status = lpPI->GetSystemParameter(&tmpReader, &addr, &lpData,100);
  if( status != SKYETEK_SUCCESS )
  {
    free(lpReader);
    goto failure;
  }
  str = SkyeTek_GetStringFromData(lpData);
  if( str != NULL )
  {
    _tcscpy(lpReader->serialNumber, str);
    SkyeTek_FreeString(str);
  }
  SkyeTek_FreeData(lpData);

  SKYETEK_Sleep(100);
  status = STR_GetSystemAddrForParm(SYS_RID,&addr,ver);
  if( status != SKYETEK_SUCCESS )
    goto failure;
  status = lpPI->GetSystemParameter(&tmpReader, &addr, &lpData,100);
  if( status != SKYETEK_SUCCESS )
	{
	  free(lpReader);
	  goto failure;
	}
	lpReader->id = (LPSKYETEK_ID)lpData;
  lpReader->internal = &SkyetekReaderImpl;
  lpReader->lpDevice = lpDevice;
  lpReader->lpProtocol = (LPSKYETEK_PROTOCOL)malloc(sizeof(SKYETEK_PROTOCOL));
  if( lpReader->lpProtocol == NULL )
  {
    free(lpReader);
    goto failure;
  }
  lpReader->lpProtocol->version = ver;
  lpReader->lpProtocol->internal = lpPI;
  _tcscpy(lpReader->manufacturer, _T("SkyeTek"));
  str = SkyeTek_GetStringFromID(lpReader->id);
  if( str == NULL )
  {
    free(lpReader);
    goto failure;
  }
  _tcscpy(lpReader->rid,str);

  /* TODO: See if Reader Name can be used */
  _stprintf(lpReader->friendly, _T("%s-%s-%s"), lpReader->manufacturer, lpReader->model, str);
  SkyeTek_FreeString(str);

  return lpReader;
failure:
  SkyeTek_FreeID(tmpReader.id);
  return NULL;
}

static int g_bootloads = 1;

LPSKYETEK_READER 
GetBootloadReader(
  LPSKYETEK_DEVICE    lpDevice, 
  LPPROTOCOLIMPL      lpPI,
  unsigned int        ver
  )
{
  LPSKYETEK_READER lpReader;

  if( lpDevice == NULL )
    return NULL;

  lpReader = (LPSKYETEK_READER)malloc(sizeof(SKYETEK_READER));
  if( lpReader == NULL )
  {
    return NULL;
  }
  memset(lpReader, 0, sizeof(SKYETEK_READER));

  _tcscpy(lpReader->firmware, _T("Bootload"));
  _tcscpy(lpReader->model, _T("??")); 
  _tcscpy(lpReader->serialNumber, _T("Bootload"));
	lpReader->id = SkyeTek_AllocateID(4);
  lpReader->internal = &SkyetekReaderImpl;
  lpReader->lpDevice = lpDevice;
  lpReader->lpProtocol = (LPSKYETEK_PROTOCOL)malloc(sizeof(SKYETEK_PROTOCOL));
  if( lpReader->lpProtocol == NULL )
  {
    free(lpReader);
    return NULL;
  }
  lpReader->lpProtocol->version = ver;
  lpReader->lpProtocol->internal = lpPI;
  _tcscpy(lpReader->manufacturer, _T("SkyeTek"));
  _tcscpy(lpReader->rid, _T("00000000"));
  lpReader->isBootload = 0x01;
  _stprintf(lpReader->friendly, _T("Bootload-%d"), g_bootloads++);

  return lpReader;
}


SKYETEK_STATUS 
SkyetekReaderFactory_CreateReader(
  LPSKYETEK_DEVICE    device, 
  LPSKYETEK_READER    *reader
  )
{
  LPSKYETEK_READER lpReader;
  LPDEVICEIMPL lpDI;
  unsigned int ix = 0;
  unsigned int ver = 0;

	if( device == NULL || device->readFD == 0 || device->internal == NULL )
    return SKYETEK_INVALID_PARAMETER; /*BAD_DEVICE*/
  
  lpDI = (LPDEVICEIMPL)device->internal;
  ver = GetReaderVersion(device);
  if( ver == 0 && _tcscmp(device->type,SKYETEK_USB_DEVICE_TYPE) == 0 )
  {
    lpDI->Flush(device);
    lpDI->Close(device);
    lpDI->Open(device);
    ver = GetReaderVersion(device);
  }

  // handle different versions
  if( ver == 2 ) 
  {
    lpReader = GetReader(device,&STPV2Impl,ver);
  } 
  else if( ver == 3 )
  {
    lpReader = GetReader(device,&STPV3Impl,ver);
  }
  else if( ver == -1 )
  {
    // bootload mode
    lpReader = GetBootloadReader(device,&STPV3Impl,3);
  }
  else
  {    
    return SKYETEK_INVALID_PARAMETER;
  }

  if( lpReader == NULL )
    return SKYETEK_INVALID_PARAMETER;

  *reader = lpReader;
  return SKYETEK_SUCCESS;
}

unsigned int 
SkyetekReaderFactory_DiscoverReaders(
  LPSKYETEK_DEVICE    *devices, 
  unsigned int        deviceCount, 
  LPSKYETEK_READER    **readers
  )
{
  unsigned int readerCount, ix, iy;
  LPSKYETEK_READER lpReader;
  LPDEVICEIMPL lpDI;
  unsigned char found = 0;
  
  if((readers == NULL) || (*readers != NULL))
    return 0;
  if( deviceCount < 1 ) 
    return 0;
  
  readerCount = 0;
  
  for(ix = 0; ix < deviceCount; ix++)
  {
    lpReader = NULL;
    lpDI = (LPDEVICEIMPL)devices[ix]->internal;
    if( lpDI == NULL )
      continue;
    found = 0;
		if( lpDI->Open(devices[ix]) == SKYETEK_SUCCESS )
		{
      if( _tcscmp(devices[ix]->type,SKYETEK_SERIAL_DEVICE_TYPE) == 0 )
      {
        for( iy = 0; iy < NUM_SERIAL_DISCOVERY_SETTINGS; iy++ )
        {
          SkyeTek_Debug(_T("Attempting at baud %d\n"), SerialDiscoverySettings[iy].baudRate);
          SerialDevice_SetOptions(devices[ix],&SerialDiscoverySettings[iy]);
			    if(SkyetekReaderFactory_CreateReader(devices[ix], &lpReader) == SKYETEK_SUCCESS)
			    {
					    *readers = (LPSKYETEK_READER*)realloc(*readers, (readerCount + 1)*sizeof(LPSKYETEK_READER));
					    (*readers)[readerCount] = lpReader;
					    readerCount++;
              found = 1;
              break;
			    }
        }
      }
      else
      {
			  if(SkyetekReaderFactory_CreateReader(devices[ix], &lpReader) == SKYETEK_SUCCESS)
			  {
					  *readers = (LPSKYETEK_READER*)realloc(*readers, (readerCount + 1)*sizeof(LPSKYETEK_READER));
					  (*readers)[readerCount] = lpReader;
					  readerCount++;
            found = 1;
			  }
      }
      if( !found )
      {
        lpDI->Close(devices[ix]);
      }
		}
  }
	
  return readerCount;
}

int 
SkyetekReaderFactory_FreeReader(
  LPSKYETEK_READER lpReader
  )
{
  if( lpReader == NULL || lpReader->internal == NULL )
    return 0;
  if( lpReader->internal == &SkyetekReaderImpl )
  {
    free(lpReader);
    return 1;
  }
  return 0;
}

void 
SkyetekReaderFactory_FreeReaders(
    LPSKYETEK_READER *readers,
    unsigned int count
    )
{
  unsigned int ix = 0;
	if(readers == NULL)
		return;
  for( ix = 0; ix < count; ix++ )
  {
    if( readers[ix] != NULL )
    {
      if( readers[ix]->internal == &SkyetekReaderImpl )
      {
        SkyetekReaderFactory_FreeReader(readers[ix]);
        readers[ix] = NULL;
      }
    }
  }
}

READER_FACTORY SkyetekReaderFactory = {
  SkyetekReaderFactory_DiscoverReaders,
  SkyetekReaderFactory_FreeReaders,
  SkyetekReaderFactory_CreateReader,
  SkyetekReaderFactory_FreeReader
};


