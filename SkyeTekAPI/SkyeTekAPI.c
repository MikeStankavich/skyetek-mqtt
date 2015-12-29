/**
 * SkyeTekAPI.c
 * Copyright � 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the SkyeTek C-API.
 */
#include "SkyeTekAPI.h"
#include "Device/DeviceFactory.h"
#include "Device/Device.h"
#include "Device/SerialDevice.h"
#include "Reader/ReaderFactory.h"
#include "Reader/Reader.h"
#include "Tag/TagFactory.h"
#include "Tag/Tag.h"
#include "Protocol/Protocol.h"
#include "Protocol/utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#ifndef __APPLE__
#include <malloc.h>
#endif

#ifdef WIN32
#ifndef WINCE
#include <io.h>
#endif
#else
#include <unistd.h>
#endif

/****************************************************
 * DISCOVERY, CREATION AND FREEING IMPLEMENTATIONS
 ****************************************************/

SKYETEK_API unsigned int 
SkyeTek_DiscoverDevices(
  LPSKYETEK_DEVICE    **lpDevices
  )
{
  if( lpDevices == NULL )
    return 0;
  else 
    return DiscoverDevicesImpl(lpDevices);
}

SKYETEK_API void 
SkyeTek_FreeDevices(
  LPSKYETEK_DEVICE    *lpDevices,
  unsigned int        count
  )
{
  if( lpDevices == NULL || count == 0 )
    return;
  FreeDevicesImpl(lpDevices,count);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CreateDevice(
  TCHAR              *address, 
  LPSKYETEK_DEVICE  *lpDevice
  )
{
	if( address == NULL || lpDevice == NULL )
		return SKYETEK_INVALID_PARAMETER;
	else
		return CreateDeviceImpl(address,lpDevice);
}

SKYETEK_API void 
SkyeTek_FreeDevice(
  LPSKYETEK_DEVICE    lpDevice
  )
{
  if( lpDevice == NULL )
    return;
  FreeDeviceImpl(lpDevice);
}

/**
 * Open the device for direct reading
 * @param lpDevice Device to free
 */
SKYETEK_API FILE* 
SkyeTek_OpenDeviceStream(
    LPSKYETEK_DEVICE   lpDevice
    )
{
  if (lpDevice == NULL)
    return NULL;
#ifdef WIN32
#ifdef WINCE
  return NULL;
#else
  return _fdopen(_dup(_open_osfhandle((int)lpDevice->readFD,0)), "rb");
#endif
#else
  return fdopen(dup(lpDevice->readFD), "rb"); 
#endif
}

SKYETEK_API unsigned int 
SkyeTek_DiscoverReaders(
  LPSKYETEK_DEVICE    *lpDevices, 
  unsigned int        deviceCount, 
  LPSKYETEK_READER    **lpReaders
  )
{
  if( lpDevices == NULL || deviceCount <= 0 || lpReaders == NULL )
    return 0;
  else
    return DiscoverReadersImpl(lpDevices,deviceCount,lpReaders);
}

SKYETEK_API void 
SkyeTek_FreeReaders(
  LPSKYETEK_READER   *lpReaders,
  unsigned int       count
  )
{
  if( lpReaders == NULL || count == 0 )
    return;
  FreeReadersImpl(lpReaders,count);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CreateReader(
  LPSKYETEK_DEVICE    lpDevice, 
  LPSKYETEK_READER    *lpReader
  )
{
	if( lpDevice == NULL || lpReader == NULL )
		return SKYETEK_INVALID_PARAMETER;
	else
		return CreateReaderImpl(lpDevice,lpReader);
}

SKYETEK_API void 
SkyeTek_FreeReader(
    LPSKYETEK_READER   lpReader
    )
{
  FreeReaderImpl(lpReader);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CreateTag(
    SKYETEK_TAGTYPE     type,
    LPSKYETEK_ID        lpId,
    LPSKYETEK_TAG       *lpTag
    )
{
  return CreateTagImpl(type,lpId,lpTag);
}

SKYETEK_API LPSKYETEK_TAG 
SkyeTek_DuplicateTag(
    LPSKYETEK_TAG       lpTag
    )
{
  if( lpTag == NULL )
    return NULL;
  return DuplicateTagImpl(lpTag);
}

SKYETEK_API void 
SkyeTek_FreeTag(
    LPSKYETEK_TAG   tag
    )
{
  if( tag == NULL )
    return;
  FreeTagImpl(tag);
}

SKYETEK_API LPSKYETEK_DATA 
SkyeTek_AllocateData(
    int size
    )
{
  LPSKYETEK_DATA data = (LPSKYETEK_DATA)malloc(sizeof(SKYETEK_DATA));
  if( data == NULL )
    return NULL;
  data->size = size;
  data->data = NULL;
  if( size > 0 )
    data->data = (unsigned char *)malloc(data->size * sizeof(unsigned char));
  if( data->data != NULL )
      memset(data->data,0,data->size*sizeof(unsigned char));
  return data;
}

SKYETEK_API void 
SkyeTek_FreeData(
    LPSKYETEK_DATA     data
    )
{
    if( data == NULL )
        return;
    if( data->data != NULL )
        free(data->data);
    data->data = NULL;
    data->size = 0;
    free(data);
}

SKYETEK_API LPSKYETEK_ID 
SkyeTek_AllocateID(
    int length
    )
{
  LPSKYETEK_ID id = (LPSKYETEK_ID)malloc(sizeof(SKYETEK_ID));
  if( id == NULL )
    return NULL;
  id->length = length;
  id->id = NULL;
  if( length > 0 )
    id->id = (unsigned char *)malloc(id->length * sizeof(unsigned char));
  if( id->id != NULL )
      memset(id->id,0,id->length*sizeof(unsigned char));
  return id;
}

SKYETEK_API void 
SkyeTek_FreeID(
    LPSKYETEK_ID     id
    )
{
    if( id == NULL )
        return;
    if( id->id != NULL )
        free(id->id);
    id->id = NULL;
    id->length = 0;
    free(id);
}

SKYETEK_API LPSKYETEK_STRING 
SkyeTek_AllocateString(
	unsigned int size
)
{
    TCHAR *str = NULL;
    
	if(size == 0)
		return NULL;
    
	str = (TCHAR *)malloc(size * sizeof(TCHAR));
    
	if(str != NULL)
        memset(str, 0, size*sizeof(TCHAR));
    
	return str;
}

SKYETEK_API void 
SkyeTek_FreeString(
	LPSKYETEK_STRING str
)
{
    if(str == NULL)
        return;

    free(str);
}

SKYETEK_API LPSKYETEK_STRING 
SkyeTek_GetStringFromData(
	LPSKYETEK_DATA data
)
{
    unsigned int size;
    unsigned int i;
    LPSKYETEK_STRING str;
	  TCHAR *ptr;

    if( data == NULL || data->data == NULL )
        return NULL;

    size = (data->size << 1) + 1;

    str = SkyeTek_AllocateString(size);
   
	for(i = 0, ptr = str; i < data->size; i++, ptr+=2)
    {
      _stprintf(ptr,_T("%02X"),data->data[i]);
    }

    return str;
}

SKYETEK_API LPSKYETEK_DATA 
SkyeTek_GetDataFromString(
	LPSKYETEK_STRING str
)
{
  LPSKYETEK_DATA data;
  unsigned int len = 0;
  unsigned int size;
  unsigned int i, j;
	TCHAR buffer[3];

  if(str == NULL)
      return NULL;

  len = _tcslen(str);
  size = len >> 1;

  data = SkyeTek_AllocateData(size);

  if(data == NULL)
    return NULL;

  memset(buffer, 0, sizeof(buffer));
	
	for( i = 0, j = 0; i < size && j < len; i++, j += 2)
  {
		buffer[0] = str[j];
		buffer[1] = str[j+1];
 		data->data[i] = (unsigned char)_tcstoul(buffer, NULL, 16);
  }

  return data;
}

SKYETEK_API LPSKYETEK_STRING
SkyeTek_GetStringFromID(
	LPSKYETEK_ID     id
)
{
  return SkyeTek_GetStringFromData((LPSKYETEK_DATA)id);
}

SKYETEK_API LPSKYETEK_ID 
SkyeTek_GetIDFromString(
	LPSKYETEK_STRING str
)
{
	return (LPSKYETEK_ID)SkyeTek_GetDataFromString(str);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CopyData(
    LPSKYETEK_DATA     lpDataTo,
    LPSKYETEK_DATA     lpDataFrom
    )
{
  if( lpDataFrom == NULL || lpDataTo == NULL || lpDataTo->data == NULL )
    return SKYETEK_INVALID_PARAMETER;
  return SkyeTek_CopyBuffer(lpDataTo, lpDataFrom->data, lpDataFrom->size);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CopyBuffer(
    LPSKYETEK_DATA     lpData,
    unsigned char      *buffer,
    unsigned int       size
    )
{
  unsigned int ix;
  unsigned int max = 0;
  
  if( buffer == NULL || lpData == NULL || lpData->data == NULL )
    return SKYETEK_INVALID_PARAMETER;
  max = (size > lpData->size ? lpData->size : size);
  for( ix = 0; ix < max; ix++ )
    lpData->data[ix] = buffer[ix];
  return SKYETEK_SUCCESS;
}

/****************************************************
 * READER IMPLEMENTATIONS
 ****************************************************/

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SelectTags(
    LPSKYETEK_READER            lpReader, 
    SKYETEK_TAGTYPE             tagType, 
    SKYETEK_TAG_SELECT_CALLBACK callback, 
    unsigned char               inv, 
    unsigned char               loop, 
    void                        *user
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->SelectTags(lpReader,tagType,callback,inv,loop,user);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetTags(
    LPSKYETEK_READER   lpReader, 
    SKYETEK_TAGTYPE    tagType, 
    LPSKYETEK_TAG      **lpTags, 
    unsigned short     *count
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;

  return lpri->GetTags(lpReader,tagType,lpTags,count);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetTagsWithMask(
    LPSKYETEK_READER   lpReader, 
    SKYETEK_TAGTYPE    tagType, 
	LPSKYETEK_ID	   lpTagIdMask,
    LPSKYETEK_TAG      **lpTags, 
    unsigned short     *count
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->GetTagsWithMask(lpReader,tagType,lpTagIdMask,lpTags,count);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_FreeTags(
    LPSKYETEK_READER  lpReader,
    LPSKYETEK_TAG     *lpTags,
    unsigned short    count
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;

  return lpri->FreeTags(lpReader,lpTags,count);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_StoreKey(
    LPSKYETEK_READER     lpReader,
    SKYETEK_TAGTYPE      type,
    LPSKYETEK_KEY        lpKey
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->StoreKey(lpReader,type,lpKey);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_LoadKey(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_KEY        lpKey
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->LoadKey(lpReader,lpKey);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_LoadDefaults(
    LPSKYETEK_READER     lpReader
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->LoadDefaults(lpReader);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ResetDevice(
    LPSKYETEK_READER     lpReader
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->ResetDevice(lpReader);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_Bootload(
    LPSKYETEK_READER     lpReader
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->Bootload(lpReader);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetSystemParameter(
    LPSKYETEK_READER             lpReader, 
    SKYETEK_SYSTEM_PARAMETER     parameter,
    LPSKYETEK_DATA               *lpData
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->GetSystemParameter(lpReader,parameter,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SetSystemParameter(
    LPSKYETEK_READER             lpReader, 
    SKYETEK_SYSTEM_PARAMETER     parameter, 
    LPSKYETEK_DATA               lpData
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->SetSystemParameter(lpReader,parameter,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetDefaultSystemParameter(
    LPSKYETEK_READER             lpReader, 
    SKYETEK_SYSTEM_PARAMETER     parameter, 
    LPSKYETEK_DATA               *lpData
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->GetDefaultSystemParameter(lpReader,parameter,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SetDefaultSystemParameter(
    LPSKYETEK_READER             lpReader, 
    SKYETEK_SYSTEM_PARAMETER     parameter, 
    LPSKYETEK_DATA               lpData
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->SetDefaultSystemParameter(lpReader,parameter,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_AuthenticateReader(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_KEY        lpKey
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->AuthenticateReader(lpReader,lpKey);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_EnableDebug(
    LPSKYETEK_READER     lpReader
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->EnableDebug(lpReader);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_DisableDebug(
    LPSKYETEK_READER     lpReader
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->DisableDebug(lpReader);
}

/**
 * Get debug messages from reader.
 * @param lpReader Reader to execute command on
 */
SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetDebugMessages(LPSKYETEK_READER     lpReader,
                         LPSKYETEK_DATA      *lpData)
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->GetDebugMessages(lpReader,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_UploadFirmware(
    LPSKYETEK_READER                      lpReader, 
    TCHAR                                  *file, 
    unsigned char                         defaultsOnly,
    SKYETEK_FIRMWARE_UPLOAD_CALLBACK      callback, 
    void                                  *user
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->UploadFirmware(lpReader,file,defaultsOnly,callback,user);
}




/****************************************************
 * TAG IMPLEMENTATIONS
 ****************************************************/

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SelectTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->SelectTag(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ReadTagData(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char        decrypt,
    unsigned char        hmac,
    LPSKYETEK_DATA       *lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ReadTagData(lpReader,lpTag,lpAddr,decrypt,hmac,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_WriteTagData(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr, 
    unsigned char        encrypt,
    unsigned char        hmac,
    LPSKYETEK_DATA       lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->WriteTagData(lpReader,lpTag,lpAddr,encrypt,hmac,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ReadTagConfig(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       *lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ReadTagConfig(lpReader,lpTag,lpAddr,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_WriteTagConfig(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr, 
    LPSKYETEK_DATA       lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->WriteTagConfig(lpReader,lpTag,lpAddr,lpData);
}

SKYETEK_API SKYETEK_STATUS SkyeTek_LockTagBlock(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->LockTagBlock(lpReader,lpTag,lpAddr,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ActivateTagType(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ActivateTagType(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_DeactivateTagType(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->DeactivateTagType(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SetTagBitRate(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    unsigned char        rate
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->SetTagBitRate(lpReader,lpTag,rate);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetTagInfo(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_MEMORY     lpMemory
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetTagInfo(lpReader,lpTag,lpMemory);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetLockStatus(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char        *status
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetLockStatus(lpReader,lpTag,lpAddr,status);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_KillTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_KEY        lpKey
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->KillTag(lpReader,lpTag,lpKey);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ReviveTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ReviveTag(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_EraseTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->EraseTag(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_FormatTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->FormatTag(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_DeselectTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->DeselectTag(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_AuthenticateTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_KEY        lpKey
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->AuthenticateTag(lpReader,lpTag,lpKey);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SendTagPassword(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->SendTagPassword(lpReader,lpTag,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetApplicationIDs(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ID         **lpIds,
    unsigned int         *count
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetApplicationIDs(lpReader,lpTag,lpIds,count);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SelectApplication(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ID         lpId
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->SelectApplication(lpReader,lpTag,lpId);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CreateApplication(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ID         lpId,
    LPSKYETEK_APP_KEY_SETTINGS  lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->CreateApplication(lpReader,lpTag,lpId,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_DeleteApplication(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ID         lpId
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->DeleteApplication(lpReader,lpTag,lpId);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetFileIDs(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ID         **lpFiles,
    unsigned int         *count
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetFileIDs(lpReader,lpTag,lpFiles,count);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SelectFile(
    LPSKYETEK_READER    lpReader,
    LPSKYETEK_TAG       lpTag,
    LPSKYETEK_ID        lpFile
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->SelectFile(lpReader,lpTag,lpFile);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CreateDataFile(
    LPSKYETEK_READER             lpReader,
    LPSKYETEK_TAG                lpTag,
    LPSKYETEK_ID                 lpFile,
    LPSKYETEK_DATA_FILE_SETTINGS lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->CreateDataFile(lpReader,lpTag,lpFile,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CreateValueFile(
    LPSKYETEK_READER                 lpReader,
    LPSKYETEK_TAG                    lpTag,
    LPSKYETEK_ID                     lpFile,
    LPSKYETEK_VALUE_FILE_SETTINGS    lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->CreateValueFile(lpReader,lpTag,lpFile,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CreateRecordFile(
    LPSKYETEK_READER                 lpReader,
    LPSKYETEK_TAG                    lpTag,
    LPSKYETEK_ID                     lpFile,
    LPSKYETEK_RECORD_FILE_SETTINGS   lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->CreateRecordFile(lpReader,lpTag,lpFile,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetCommonFileSettings(
    LPSKYETEK_READER             lpReader,
    LPSKYETEK_TAG                lpTag,
    LPSKYETEK_ID                 lpFile,
    LPSKYETEK_FILE_SETTINGS      lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetCommonFileSettings(lpReader,lpTag,lpFile,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetDataFileSettings(
    LPSKYETEK_READER                 lpReader,
    LPSKYETEK_TAG                    lpTag,
    LPSKYETEK_ID                     lpFile,
    LPSKYETEK_DATA_FILE_SETTINGS     lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetDataFileSettings(lpReader,lpTag,lpFile,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetValueFileSettings(
    LPSKYETEK_READER                 lpReader,
    LPSKYETEK_TAG                    lpTag,
    LPSKYETEK_ID                     lpFile,
    LPSKYETEK_VALUE_FILE_SETTINGS    lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetValueFileSettings(lpReader,lpTag,lpFile,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetRecordFileSettings(
    LPSKYETEK_READER                 lpReader,
    LPSKYETEK_TAG                    lpTag,
    LPSKYETEK_ID                     lpFile,
    LPSKYETEK_RECORD_FILE_SETTINGS   lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetRecordFileSettings(lpReader,lpTag,lpFile,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ChangeFileSettings(
    LPSKYETEK_READER             lpReader,
    LPSKYETEK_TAG                lpTag,
    LPSKYETEK_ID                 lpFile,
    LPSKYETEK_FILE_SETTINGS      lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ChangeFileSettings(lpReader,lpTag,lpFile,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ReadFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       *lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ReadFile(lpReader,lpTag,lpFile,lpAddr,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_WriteFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->WriteFile(lpReader,lpTag,lpFile,lpAddr,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_DeleteFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->DeleteFile(lpReader,lpTag,lpFile);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ClearFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ClearFile(lpReader,lpTag,lpFile);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CreditValueFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile,
    unsigned int         amount
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->CreditValueFile(lpReader,lpTag,lpFile,amount);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_DebitValueFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile,
    unsigned int         amount
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->DebitValueFile(lpReader,lpTag,lpFile,amount);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_LimitedCreditValueFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile,
    unsigned int         amount
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->LimitedCreditValueFile(lpReader,lpTag,lpFile,amount);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetValue(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile,
    unsigned int         *value
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetValue(lpReader,lpTag,lpFile,value);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ReadRecords(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       *lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ReadRecords(lpReader,lpTag,lpFile,lpAddr,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_WriteRecord(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ID         lpFile,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->WriteRecord(lpReader,lpTag,lpFile,lpAddr,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_CommitTransaction(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->CommitTransaction(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_AbortTransaction(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->AbortTransaction(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_EnableEAS(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->EnableEAS(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_DisableEAS(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->DisableEAS(lpReader,lpTag);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ScanEAS(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned char        *state
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ScanEAS(lpReader,lpTag,state);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ReadAFI(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       *lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ReadAFI(lpReader,lpTag,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_WriteAFI(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->WriteAFI(lpReader,lpTag,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ReadDSFID(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       *lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ReadDSFID(lpReader,lpTag,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_WriteDSFID(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->WriteDSFID(lpReader,lpTag,lpData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetKeyVersion(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_KEY        lpKey
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetKeyVersion(lpReader,lpTag,lpKey);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ChangeKey(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_KEY        lpNewKey,
    LPSKYETEK_KEY        lpCurrentKey
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ChangeKey(lpReader,lpTag,lpNewKey,lpCurrentKey);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetApplicationKeySettings(
    LPSKYETEK_READER            lpReader,
    LPSKYETEK_TAG               lpTag,
    LPSKYETEK_APP_KEY_SETTINGS  lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetApplicationKeySettings(lpReader,lpTag,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetMasterKeySettings(
    LPSKYETEK_READER                lpReader,
    LPSKYETEK_TAG                   lpTag,
    LPSKYETEK_MASTER_KEY_SETTINGS   lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->GetMasterKeySettings(lpReader,lpTag,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ChangeApplicationKeySettings(
    LPSKYETEK_READER             lpReader,
    LPSKYETEK_TAG                lpTag,
    LPSKYETEK_APP_KEY_SETTINGS   lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ChangeApplicationKeySettings(lpReader,lpTag,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ChangeMasterKeySettings(
    LPSKYETEK_READER                 lpReader,
    LPSKYETEK_TAG                    lpTag,
    LPSKYETEK_MASTER_KEY_SETTINGS    lpSettings
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ChangeMasterKeySettings(lpReader,lpTag,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_InitializeSecureMemoryTag(
    LPSKYETEK_READER          lpReader,
    LPSKYETEK_TAG             lpTag,
    SKYETEK_HMAC_ALGORITHM    hmac,
    LPSKYETEK_KEY             lpKeyHMAC,
    SKYETEK_CIPHER_ALGORITHM  cipher,
    LPSKYETEK_KEY             lpKeyCipher,
		int                       useKeyDerivationFunction
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->InitializeSecureMemoryTag(lpReader,lpTag,hmac,lpKeyHMAC,cipher,lpKeyCipher, useKeyDerivationFunction);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SetupSecureMemoryTag(
    LPSKYETEK_READER          lpReader,
    LPSKYETEK_TAG             lpTag,
    LPSKYETEK_KEY             lpKeyHMAC,
    LPSKYETEK_KEY             lpKeyCipher,
		int                       useKeyDerivationFunction
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->SetupSecureMemoryTag(lpReader,lpTag,lpKeyHMAC,lpKeyCipher, useKeyDerivationFunction);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_InterfaceSend(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    SKYETEK_INTERFACE    inteface,
    SKYETEK_BLOCK        block,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->InterfaceSend(lpReader,lpTag,inteface,block,lpSendData,lpRecvData);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_TransportSend(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    SKYETEK_TRANSPORT    transport,
    SKYETEK_BLOCK        block,
    LPSKYETEK_DATA       lpCla,
    LPSKYETEK_DATA       lpIns,
    LPSKYETEK_DATA       lpP1p2,
    LPSKYETEK_DATA       lpData, 
    unsigned int         le,
    LPSKYETEK_DATA       *lpRecvData
    )
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->TransportSend(lpReader,lpTag,transport,block,lpCla,
    lpIns,lpP1p2,lpData,le,lpRecvData);
}

#ifdef PASSTHROUGH_MODE_EN
/****************************************************
 * PASSTHROUGH IMPLEMENTATIONS
 ****************************************************/
SKYETEK_API SKYETEK_STATUS 
SkyeTek_PassThrough(
    LPSKYETEK_READER   lpReader,
    unsigned char     *sendData,
	unsigned int	  sendDataLen,	
	unsigned char	  *recvData,
	unsigned int	  *recvDataLen,	
	unsigned int       timeout
    )
{
	LPDEVICEIMPL pd;
	union _16Bits len;
	UINT8 temp;
	int i = 0, num = 0, bytes = 0, count = 0, totalCount = 0;
	UINT32 bytesRead;

	if( lpReader->lpDevice == NULL )
		return 0;
	
	pd = (LPDEVICEIMPL)lpReader->lpDevice->internal;
	if( pd == NULL )
		return 0;
	
	/* Send the Passthrough Command */
	pd->Write(lpReader->lpDevice, sendData, sendDataLen, 500);
	pd->Flush(lpReader->lpDevice);

	SKYETEK_Sleep(timeout);

	/* Read the response from the reader */	
	*recvDataLen = 0;
	while((bytesRead = pd->Read(lpReader->lpDevice, recvData, 1, 500)))
	{
		recvData += bytesRead;
		*recvDataLen += bytesRead;
	}

	if(*recvDataLen == 0)
		return SKYETEK_FAILURE;
	else
		return SKYETEK_SUCCESS;

}
#endif


/****************************************************
 * DEBUGGING IMPLEMENTATIONS
 ****************************************************/

SKYETEK_API TCHAR *
SkyeTek_GetStatusMessage(
  SKYETEK_STATUS status
  )
{
	switch(status)
	{
		case SKYETEK_FAILURE:
			return _T("Failure");
			break;
		case SKYETEK_SUCCESS:
			return _T("Success");
			break;
		case SKYETEK_INVALID_READER:
			return _T("Invalid reader");
			break;
		case SKYETEK_READER_PROTOCOL_ERROR:
			return _T("Reader protocol error");
			break;
		case SKYETEK_READER_IN_BOOT_LOAD_MODE:
			return _T("Reader in boot load mode");
			break;
		case SKYETEK_TAG_NOT_IN_FIELD:
			return _T("Tag not in field");
			break;
		case SKYETEK_NO_TAG_ID_MATCH:
			return _T("No tag ID match");
			break;
		case SKYETEK_READER_IO_ERROR:
			return _T("Reader I/O Error");
			break;
		case SKYETEK_INVALID_PARAMETER:
			return _T("Invalid parameter");
			break;
		case SKYETEK_TIMEOUT:
			return _T("Timeout");
			break;
		case SKYETEK_NOT_SUPPORTED:
			return _T("Not supported");
			break;
		case SKYETEK_OUT_OF_MEMORY:
			return _T("Out of memory");
			break;
		case SKYETEK_COLLISION_DETECTED:
			return _T("Collision detected");
			break;
		case SKYETEK_TAG_DATA_INTEGRITY_CHECK_FAILED:
			return _T("Integrity check failed");
			break;
		case SKYETEK_TAG_BLOCKS_LOCKED:
			return _T("Tag blocks locked");
			break;
		case SKYETEK_NOT_AUTHENTICATED:
			return _T("Not authenticated");
			break;
		case SKYETEK_TAG_DATA_RATE_NOT_SUPPORTED:
			return _T("Tag data rate not supported");
			break;
		case SKYETEK_INVALID_KEY_FOR_AUTHENTICATION:
			return _T("Invalid key for authentication");
			break;
		case SKYETEK_NO_APPLICATION_PRESENT:
			return _T("No application present");
			break;
		case SKYETEK_FILE_NOT_FOUND:
			return _T("File not found");
			break;
		case SKYETEK_NO_FILE_SELECTED:
			return _T("No file selected");
			break;
		case SKYETEK_INVALID_FILE_TYPE:
			return _T("Invalid file type");
			break;
		case SKYETEK_INVALID_KEY_NUMBER:
			return _T("Invalid key number");
			break;
		case SKYETEK_INVALID_KEY_LENGTH:
			return _T("Invalid key length");
			break;
		case SKYETEK_UNKOWN_ERROR:
			return _T("Unknown reader error");
			break;
		case SKYETEK_INVALID_COMMAND:
			return _T("Invalid command");
			break;
		case SKYETEK_INVALID_CRC:
			return _T("Invalid CRC");
			break;
		case SKYETEK_INVALID_MESSAGE_LENGTH:
			return _T("Invalid message length");
			break;
		case SKYETEK_INVALID_ADDRESS:
			return _T("Invalid address");
			break;
		case SKYETEK_INVALID_FLAGS:
			return _T("Invalid protocol flags");
			break;
		case SKYETEK_INVALID_NUMBER_OF_BLOCKS:
			return _T("Invalid number of blocks");
			break;
		case SKYETEK_NO_ANTENNA_DETECTED:
			return _T("No antenna detected");
			break;
		case SKYETEK_INVALID_TAG_TYPE:
			return _T("Invalid tag type");
			break;
		case SKYETEK_ENCRYPT_TAG_DATA_FAIL:
			return _T("Attempt to encrypt tag data failed");
			break;
		case SKYETEK_DECRYPT_TAG_DATA_FAIL:
			return _T("Attempt to decrypt tag data failed");
			break;
		case SKYETEK_INVALID_SIGNATURE_HMAC:
			return _T("Invalid signature HMAC");
			break;
		case SKYETEK_INVALID_ASCII_CHAR:
			return _T("Invalid ASCII character");
			break;
		case SKYETEK_INVALID_DATA_LEN:
			return _T("Invalid data length");
			break;
		case SKYETEK_INVALID_ENCODING:
			return _T("Invalid encoding");
			break;
		case SKYETEK_INVALID_ARGUMENT:
			return _T("Invalid argument");
			break;
		case SKYETEK_INVALID_SESSION:
			return _T("Invalid session");
			break;
		case SKYETEK_FIRMWARE_CANCELED:
			return _T("Firmware update canceled");
			break;
		case SKYETEK_FIRMWARE_BAD_FILE:
			return _T("Error reading firmware file");
			break;
		case SKYETEK_FIRMWARE_READER_ERROR:
			return _T("Error updating firmware on reader");
			break;
		default:
			return _T("Unknown error");
	}
}

SKYETEK_DEBUG_CALLBACK gDebugger = NULL;
SKYETEK_API void 
SkyeTek_SetDebugger(
  SKYETEK_DEBUG_CALLBACK callback
  )
{
	if( callback == NULL )
		gDebugger = NULL;
	else
		gDebugger = callback;
}

void 
SkyeTek_Debug(
  TCHAR * sz, 
  ...
  )
{
	TCHAR gDbgMsg[2048];
	va_list args; 

	if( gDebugger == NULL ) 
		return;
	if( sz == NULL ) 
		return;
	memset(gDbgMsg,0,2048*sizeof(TCHAR));
	va_start( args, sz );
	_vsntprintf(gDbgMsg, 2047, sz, args); 
	va_end( args );
	gDebugger(gDbgMsg);
}

/********************************************************************************
 * Raw Device 
 ********************************************************************************/
SKYETEK_API SKYETEK_STATUS
SkyeTek_OpenDevice(
  LPSKYETEK_DEVICE    lpDevice
  )
{
  LPDEVICEIMPL lpDI;
  if( lpDevice == NULL || lpDevice->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpDI = (LPDEVICEIMPL)lpDevice->internal;
  return lpDI->Open(lpDevice);
}

SKYETEK_API int
SkyeTek_ReadDevice(
  LPSKYETEK_DEVICE    lpDevice,
  unsigned char       *data,
  unsigned int        size,
  unsigned int        timeout
  )
{
  LPDEVICEIMPL lpDI;
  if( lpDevice == NULL || lpDevice->internal == NULL )
    return -1;
  lpDI = (LPDEVICEIMPL)lpDevice->internal;
  return lpDI->Read(lpDevice, data, size,timeout);
}

SKYETEK_API int
SkyeTek_WriteDevice(
  LPSKYETEK_DEVICE    lpDevice,
  unsigned char       *data,
  unsigned int        size,
  unsigned int        timeout
  )
{
  LPDEVICEIMPL lpDI;
  if( lpDevice == NULL || lpDevice->internal == NULL )
    return -1;
  lpDI = (LPDEVICEIMPL)lpDevice->internal;
  return lpDI->Write(lpDevice, data, size, timeout);
}

SKYETEK_API void
SkyeTek_FlushDevice(
  LPSKYETEK_DEVICE    lpDevice
  )
{
  LPDEVICEIMPL lpDI;
  if( lpDevice == NULL || lpDevice->internal == NULL )
    return;
  lpDI = (LPDEVICEIMPL)lpDevice->internal;
  lpDI->Flush(lpDevice);
}

SKYETEK_API SKYETEK_STATUS
SkyeTek_CloseDevice(
  LPSKYETEK_DEVICE    lpDevice
  )
{
  LPDEVICEIMPL lpDI;
  if( lpDevice == NULL || lpDevice->internal == NULL )
    return (SKYETEK_STATUS)-1;
  lpDI = (LPDEVICEIMPL)lpDevice->internal;
  return lpDI->Close(lpDevice);
}

SKYETEK_API SKYETEK_STATUS
SkyeTek_SetAdditionalTimeout(
  LPSKYETEK_DEVICE    lpDevice,
  unsigned int        timeout
  )
{
  LPDEVICEIMPL lpDI;
  if( lpDevice == NULL || lpDevice->internal == NULL )
    return (SKYETEK_STATUS)-1;
  lpDI = (LPDEVICEIMPL)lpDevice->internal;
  return lpDI->SetAdditionalTimeout(lpDevice,timeout);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_SetSerialOptions(
  LPSKYETEK_DEVICE            device, 
  LPSKYETEK_SERIAL_SETTINGS   lpSettings
  )
{
  return SerialDevice_SetOptions(device,lpSettings);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_GetSerialOptions(
  LPSKYETEK_DEVICE            device, 
  LPSKYETEK_SERIAL_SETTINGS   lpSettings
  )
{
  return SerialDevice_GetOptions(device,lpSettings);
}


/********************************************************************************
 * System Parameter Information for both STPV2 and STPV3
 ********************************************************************************/

SKYETEK_API unsigned int 
SkyeTek_GetSysParmIdCount(void)
{
  return NUM_SYSPARMDESCS;
}

SKYETEK_API TCHAR *
SkyeTek_GetSysParmIdName(
  unsigned int index
  )
{
  if(index < 0 || index >= NUM_SYSPARMDESCS)
    return NULL;
  
  return SysParmIds[index].name;
}

SKYETEK_API SKYETEK_SYSTEM_PARAMETER 
SkyeTek_GetSysParmId(
  unsigned int index
  )
{
  if(index < 0 || index >= NUM_SYSPARMDESCS)
    return SYS_FIRMWARE;
  return SysParmIds[index].parameter;
}

SKYETEK_API SKYETEK_SYSTEM_PARAMETER 
SkyeTek_GetSysParmIdFromName(
  TCHAR  *name
  )
{
  unsigned ix;

  if( name == NULL )
    return SYS_FIRMWARE;
  
  for(ix = 0; ix < NUM_SYSPARMDESCS; ix++)
	{
    if(_tcscmp(SysParmIds[ix].name,name) == 0)
      return SysParmIds[ix].parameter;
	}
  return SYS_SERIALNUMBER;
}

SKYETEK_API TCHAR *
SkyeTek_GetSysParmIdNameFromParameter(
  SKYETEK_SYSTEM_PARAMETER parameter
  )
{
  unsigned ix;
  
  for(ix = 0; ix < NUM_SYSPARMDESCS; ix++)
	{
    if(SysParmIds[ix].parameter == parameter)
      return SysParmIds[ix].name;
	}
  return _T("");
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_InitiatePayment(
    LPSKYETEK_READER         lpReader,
    LPSKYETEK_TAG            lpTag,
    SKYETEK_PAYMENT_SYSTEM  *lpPaymentSystem)
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->InitiatePayment(lpReader,lpTag,lpPaymentSystem);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ComputePayment(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    SKYETEK_TRANSACTION  transaction,
    SKYETEK_PAYMENT_SYSTEM  *lpPaymentSystem)
{
  LPTAGIMPL lpti;
  if( lpReader == NULL || lpReader->internal == NULL || 
      lpTag == NULL || lpTag->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpti = (LPTAGIMPL)lpTag->internal;
  return lpti->ComputePayment(lpReader,lpTag,transaction,lpPaymentSystem);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_EnterPaymentScanMode(
    LPSKYETEK_READER         lpReader
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->EnterPaymentScanMode(lpReader);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ScanPayments(
    LPSKYETEK_READER              lpReader,
    SKYETEK_PAYMENT_SCAN_CALLBACK callback,
    void                          *user
    )
{
  LPREADER_IMPL lpri;
  if( lpReader == NULL || lpReader->internal == NULL || callback == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lpri = (LPREADER_IMPL)lpReader->internal;
  return lpri->ScanPayments(lpReader,callback,user);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTek_ParsePaymentTrack1(
    unsigned char* rawTrack1,
		int rawTrack1length,
		LPSKYETEK_TRACK1 lpTrack1
    ) 
{
	unsigned char *p, *s;
	int l;
	
	st_memset(lpTrack1, 0, sizeof(SKYETEK_TRACK1));

	p = rawTrack1;
	l = rawTrack1length;
	
	if ((l < 9) || (p[0] != '%') || (p[1] != 'B'))
		return SKYETEK_FAILURE;
	p += 2;
	l -= 2;

	do {
		for (s = p; (l > 0) && (*p != '^'); p++, l--);
		if ((l == 0) || ((p-s+1) > sizeof(lpTrack1->primaryAccountNumber)))
			return SKYETEK_FAILURE;
		st_memcpy(lpTrack1->primaryAccountNumber, s, p-s);
		lpTrack1->primaryAccountNumber[p-s] = '\0';
		*p++;
		l--;

		for (s = p; (l > 0) && (*p != '/') && (*p != '^'); p++, l--);
		if ((l == 0) || ((p-s+1) > sizeof(lpTrack1->surname)))
			return SKYETEK_FAILURE;
		st_memcpy(lpTrack1->surname, s, p-s);
		lpTrack1->surname[p-s] = '\0';
		*p++;
		l--;
		if (p[-1] == '^')
			break;

		for (s = p; (l > 0) && (*p != ' ') && (*p != '^'); p++, l--);
		if ((l == 0) || ((p-s+1) > sizeof(lpTrack1->firstName)))
			return SKYETEK_FAILURE;
		st_memcpy(lpTrack1->firstName, s, p-s);
		lpTrack1->firstName[p-s] = '\0';
		*p++;
		l--;
		if (p[-1] == '^')
			break;

		for (s = p; (l > 0) && (*p != '.') && (*p != '^'); p++, l--);
		if ((l == 0) || ((p-s+1) > sizeof(lpTrack1->middleName)))
			return SKYETEK_FAILURE;
		st_memcpy(lpTrack1->middleName, s, p-s);
		lpTrack1->middleName[p-s] = '\0';
		*p++;
		l--;
		if (p[-1] == '^')
			break;

		for (s = p; (l > 0) && (*p != '^'); p++, l--);
		if ((l == 0) || ((p-s+1) > sizeof(lpTrack1->title)))
			return SKYETEK_FAILURE;
		st_memcpy(lpTrack1->title, s, p-s);
		lpTrack1->title[p-s] = '\0';
		*p++;
		l--;
	} while(0);

	if (*p != '^') {
		st_memcpy(lpTrack1->expiration, p, 4);
		lpTrack1->expiration[4] = '\0';
		p += 4;
		l -= 4;
	} else {
		p++;
		l--;
	}
		
	if (*p != '^') {
		st_memcpy(lpTrack1->serviceCode, p, 3);
		lpTrack1->serviceCode[3] = '\0';
		p += 3;
		l -= 3;
	} else {
		p++;
		l--;
	}
	
	return SKYETEK_SUCCESS;
}

