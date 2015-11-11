/**
 * SkyeTekReader.c
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the SkyeTekReader
 */
#include "../SkyeTekAPI.h"
#include "ReaderFactory.h"
#include "Reader.h"
#include "../Device/Device.h"
#include "../Protocol/Protocol.h"
#include "../Tag/TagFactory.h"
#include <stdio.h>
#include <stdlib.h>

#define SKYETEK_TIMEOUT 500

typedef struct ST_CALLBACK_DATA
{
  SKYETEK_TAG_SELECT_CALLBACK   callback;
  void                          *user;
} ST_CALLBACK_DATA, *LPST_CALLBACK_DATA;

unsigned char 
SkyeTekReader_SelectTagsCallback(
    SKYETEK_TAGTYPE type,
    LPSKYETEK_DATA lpData,
    void  *user
    )
{
  LPST_CALLBACK_DATA lpCd;
  LPSKYETEK_TAG lpTag = NULL;
  LPSKYETEK_ID lpId;
  SKYETEK_STATUS status;
  
  if( user == NULL )
    return 0;

  lpCd = (LPST_CALLBACK_DATA)user;
  if( lpData == NULL || lpData->data == NULL || lpData->size == 0 )
  {
    return lpCd->callback(NULL,lpCd->user);
  }

  lpId = (LPSKYETEK_ID)lpData;
  status = CreateTagImpl(type, lpId, &lpTag);
  if( status != SKYETEK_SUCCESS )
    return 0;

  return lpCd->callback(lpTag,lpCd->user);
}

SKYETEK_STATUS 
SkyeTekReader_SelectTags(
    LPSKYETEK_READER            lpReader, 
    SKYETEK_TAGTYPE             tagType, 
    SKYETEK_TAG_SELECT_CALLBACK callback, 
    unsigned char               inv, 
    unsigned char               loop, 
    void                        *user
    )
{
  LPPROTOCOLIMPL lppi;
  PROTOCOL_FLAGS flags;
  ST_CALLBACK_DATA cd;

  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;
  
  flags.isInventory = inv;
  flags.isLoop = loop;
  cd.callback = callback;
  cd.user = user;

  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->SelectTags(lpReader,tagType,SkyeTekReader_SelectTagsCallback,flags,(void *)&cd,2000);
}

SKYETEK_STATUS 
SkyeTekReader_GetTags(
    LPSKYETEK_READER   lpReader, 
    SKYETEK_TAGTYPE    tagType, 
    LPSKYETEK_TAG      **lpTags, 
    unsigned short     *count
    )
{
  LPPROTOCOLIMPL lppi;
  unsigned int num;
  unsigned int ix = 0, iy = 0;
  LPTAGTYPE_ARRAY *tagTypes = NULL;
  LPSKYETEK_DATA *lpData = NULL;
  SKYETEK_STATUS status;

  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;

  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;

  status = lppi->GetTags(lpReader,tagType,&tagTypes,&lpData,&num,5000);
  if( status != SKYETEK_SUCCESS )
    return status;

  if( num > 0 )
  {
    *lpTags = (LPSKYETEK_TAG *)malloc(num * sizeof(LPSKYETEK_TAG));
    if( *lpTags == NULL )
    {
      status = SKYETEK_OUT_OF_MEMORY;
      goto failure;
    }

    for( ix = 0; ix < num; ix++ )
    {
      status = CreateTagImpl(tagTypes[ix]->type, (LPSKYETEK_ID)lpData[ix], &((*lpTags)[ix]));
      if( status != SKYETEK_SUCCESS )
        goto failure;
    }
  }

  *count = num;
  status = SKYETEK_SUCCESS;
  goto cleanup;

failure:
  /* Clean up partially created tags */
  for( iy = 0; iy < ix; iy ++ )
    FreeTagImpl((*lpTags)[iy]);
cleanup:
  /* Clean up memory from protocol */
  if( lpData != NULL )
  {
    for( ix = 0; ix < num; ix++ )
    {
      SkyeTek_FreeData(lpData[ix]);
    }
    free(lpData);
  }
  if( tagTypes != NULL )
    free(tagTypes);
  return status;
}

SKYETEK_STATUS 
SkyeTekReader_GetTagsWithMask(
    LPSKYETEK_READER   lpReader, 
    SKYETEK_TAGTYPE    tagType, 
    LPSKYETEK_ID	   lpTagIdMask,
    LPSKYETEK_TAG      **lpTags, 
    unsigned short     *count
    )
{
  LPPROTOCOLIMPL lppi;
  unsigned int num;
  unsigned int ix = 0, iy = 0;
  LPTAGTYPE_ARRAY *tagTypes = NULL;
  LPSKYETEK_DATA *lpData = NULL;
  SKYETEK_STATUS status;

  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;

  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;

  status = lppi->GetTagsWithMask(lpReader,tagType,lpTagIdMask,&tagTypes,&lpData,&num,5000);
  if( status != SKYETEK_SUCCESS )
    return status;

  if( num > 0 )
  {
    *lpTags = (LPSKYETEK_TAG *)malloc(num * sizeof(LPSKYETEK_TAG));
    if( *lpTags == NULL )
    {
      status = SKYETEK_OUT_OF_MEMORY;
      goto failure;
    }

    for( ix = 0; ix < num; ix++ )
    {
      status = CreateTagImpl(tagTypes[ix]->type, (LPSKYETEK_ID)lpData[ix], &((*lpTags)[ix]));
      if( status != SKYETEK_SUCCESS )
        goto failure;
    }
  }

  *count = num;
  status = SKYETEK_SUCCESS;
  goto cleanup;

failure:
  /* Clean up partially created tags */
  for( iy = 0; iy < ix; iy ++ )
    FreeTagImpl((*lpTags)[iy]);
cleanup:
  /* Clean up memory from protocol */
  if( lpData != NULL )
  {
    for( ix = 0; ix < num; ix++ )
    {
      SkyeTek_FreeData(lpData[ix]);
    }
    free(lpData);
  }
  if( tagTypes != NULL )
    free(tagTypes);
  return status;
}

SKYETEK_STATUS 
SkyeTekReader_FreeTags(
    LPSKYETEK_READER   lpReader, 
    LPSKYETEK_TAG      *lpTags, 
    unsigned short     count
    )
{
  unsigned int ix;
  if( lpTags == NULL || count < 1 )
    return SKYETEK_FAILURE;

  for( ix = 0; ix < count; ix++ )
    FreeTagImpl(lpTags[ix]);

  free(lpTags);
  return SKYETEK_SUCCESS;
}

SKYETEK_STATUS 
SkyeTekReader_StoreKey(
    LPSKYETEK_READER     lpReader,
    SKYETEK_TAGTYPE      type,
    LPSKYETEK_KEY        lpKey
    )
{
  LPPROTOCOLIMPL lppi;
  SKYETEK_ADDRESS addr;

  if( lpReader == NULL || lpReader->lpProtocol == NULL || 
    lpReader->lpDevice == NULL || lpKey == NULL || lpKey->lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;

  addr.start = lpKey->number;
  addr.blocks = 1;

  return lppi->StoreKey(lpReader,type,&addr,lpKey->lpData,SKYETEK_TIMEOUT);
}

SKYETEK_STATUS 
SkyeTekReader_LoadKey(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_KEY        lpKey
    )
{
  LPPROTOCOLIMPL lppi;
  SKYETEK_ADDRESS addr;

  if( lpReader == NULL || lpReader->lpProtocol == NULL || 
    lpReader->lpDevice == NULL || lpKey == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;

  addr.start = lpKey->number;
  addr.blocks = 1;

  return lppi->LoadKey(lpReader,&addr,500);
}

SKYETEK_STATUS 
SkyeTekReader_LoadDefaults(
    LPSKYETEK_READER     lpReader
    )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->LoadDefaults(lpReader,SKYETEK_TIMEOUT);
}

SKYETEK_STATUS 
SkyeTekReader_ResetDevice(
    LPSKYETEK_READER     lpReader
    )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->ResetDevice(lpReader,2000);
}

SKYETEK_STATUS 
SkyeTekReader_Bootload(
    LPSKYETEK_READER     lpReader
    )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->Bootload(lpReader,100);
}

SKYETEK_STATUS 
STR_GetSystemAddrForParm(
  SKYETEK_SYSTEM_PARAMETER  parameter,
  LPSKYETEK_ADDRESS         lpAddr,
  unsigned int              version
  )
{
  if( lpAddr == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( version == 3 )
  {
    lpAddr->start = parameter;
    switch(parameter)
    {
      case SYS_SERIALNUMBER:
        lpAddr->blocks = 4;
        /* lpAddr->blocks = 1;   Change back to 4 */
        break;
      case SYS_FIRMWARE:
        lpAddr->blocks = 4;
        /* lpAddr->blocks = 1;  Change back to 4 */
        break;
      case SYS_RID:
        /* lpAddr->start = 0x0002; Remove! */
        lpAddr->blocks = 4;
        break;
      case SYS_RADIO_LOCK: 
      case SYS_CURRENT_FREQUENCY:  
      case SYS_START_FREQUENCY: 
      case SYS_STOP_FREQUENCY: 
      case SYS_HOP_CHANNEL_SPACING:
      case SYS_HARDWARE:
        lpAddr->blocks = 4;
        /* lpAddr->blocks = 1;   Change back to 4 */
        break;
      case SYS_MARK_DAC_VALUE:
      case SYS_SPACE_DAC_VALUE:
      case SYS_POWER_DETECTOR_VALUE:
      case SYS_PRODUCT:
        lpAddr->blocks = 2;
        /* lpAddr->blocks = 1;   Change back to 4 */
        break;
      case SYS_READER_NAME:
        lpAddr->blocks = 32;
        /* lpAddr->blocks = 1;   Change back to 4 */
        break;
      default:
        lpAddr->blocks = 1;
        break;
    }
  }
  else if( version == 2 )
  {
    /* Missing: SYS_COMMAND_STORE = 0x0007, 
                SYS_AUTHENTICATE = 0x0010, 
                SYS_WRITEKEYE2 = 0x0011,
                SYS_AUTHENTBITS = 0x0012,
                SYS_TEST_PARAM1 = 0x0014,
                SYS_TEST_PARAM2 = 0x0015,
                SYS_TX_POWER_LIMIT = 0x0016,
                SYS_TX_MOD_DEPTH = 0x0017,
                SYS_TAG_ORDER = 0x001B */

    lpAddr->blocks = 1;
    switch(parameter) 
    {
      case SYS_SERIALNUMBER:    /* = 0x0000, 4 bytes */
      case SYS_FIRMWARE:        /* = 0x0001, 4 bytes */
        lpAddr->start = parameter;
        break;
      case SYS_RID:             /* = 0x0004, 4 bytes */
        lpAddr->start = 0x0002;
        lpAddr->blocks = 4;
        break; 
      case SYS_BAUD:            /* = 0x0007, 1 byte */
        lpAddr->start = 0x0003;
        break;
      case SYS_OPERATING_MODE:  /* = 0x000C, 1 byte */
        lpAddr->start = 0x0004;
        break;
      case SYS_PORT_DIRECTION:  /* = 0x0008, 1 byte */
        lpAddr->start = 0x0005;
        break;
      case SYS_PORT_VALUE:      /* = 0x0009, 1 byte */
        lpAddr->start = 0x0006;
        break;
      case SYS_READER_NAME:     /* = 0x0005, 32 bytes */
        lpAddr->start = 0x0008;
        lpAddr->blocks = 4;
        break;
      case SYS_MUX_CONTROL:     /* = 0x000A, 1 byte */
        lpAddr->start = 0x0009;
        break;
      case SYS_BOOTLOAD:        /*= 0x000B,  1 byte */
        lpAddr->start = 0x000B;
        break;
      case SYS_HOST_INTERFACE:  /* = 0x0006, 1 byte */
        lpAddr->start = 0x001A;
        break;
      case SYS_HARDWARE:        /*= 0x0002,  4 bytes */
      case SYS_PRODUCT:         /*= 0x0003,  2 bytes */
      case SYS_ENCRYPTION:      /*= 0x000D,  1 byte */
      case SYS_HMAC:            /*= 0x000E,  1 byte */
      case SYS_PERIPHERAL:      /*= 0x000F,  1 byte */
      case SYS_TAG_POPULATION:  /*= 0x0010,  1 byte */
      case SYS_COMMAND_RETRY:   /*= 0x0011,  1 byte */
      case SYS_POWER_LEVEL:     /*= 0x0012,  1 byte */
        return SKYETEK_INVALID_PARAMETER;
        break;
      default:
        lpAddr->start = parameter;
        lpAddr->blocks = 1;
        break;
    } /* End Switch */
  } /* End Version 2 */
  return SKYETEK_SUCCESS;
}

SKYETEK_STATUS 
SkyeTekReader_GetSystemParameter(
    LPSKYETEK_READER             lpReader, 
    SKYETEK_SYSTEM_PARAMETER     parameter,
    LPSKYETEK_DATA               *lpData
    )
{
  LPPROTOCOLIMPL lppi;
  SKYETEK_ADDRESS addr;
  SKYETEK_STATUS st;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || 
      lpReader->lpDevice == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  if( (st = STR_GetSystemAddrForParm(parameter,&addr,lpReader->lpProtocol->version)) != SKYETEK_SUCCESS )
    return st;
  if( parameter == SYS_OPTIMAL_POWER_C1G1 || parameter == SYS_OPTIMAL_POWER_C1G2 || 
    parameter == SYS_OPTIMAL_POWER_180006B || parameter == SYS_RSSI_VALUES )
    return lppi->GetSystemParameter(lpReader,&addr,lpData,10000);
  else
    return lppi->GetSystemParameter(lpReader,&addr,lpData,SKYETEK_TIMEOUT);
}

SKYETEK_STATUS 
SkyeTekReader_SetSystemParameter(
    LPSKYETEK_READER             lpReader, 
    SKYETEK_SYSTEM_PARAMETER     parameter, 
    LPSKYETEK_DATA               lpData
    )
{
  LPPROTOCOLIMPL lppi;
  SKYETEK_ADDRESS addr;
  SKYETEK_STATUS st;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || 
      lpReader->lpDevice == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  if( (st = STR_GetSystemAddrForParm(parameter,&addr,lpReader->lpProtocol->version)) != SKYETEK_SUCCESS )
    return st;
  return lppi->SetSystemParameter(lpReader,&addr,lpData,SKYETEK_TIMEOUT);
}

SKYETEK_STATUS 
SkyeTekReader_GetDefaultSystemParameter(
    LPSKYETEK_READER             lpReader, 
    SKYETEK_SYSTEM_PARAMETER     parameter, 
    LPSKYETEK_DATA               *lpData
    )
{
  LPPROTOCOLIMPL lppi;
  SKYETEK_ADDRESS addr;
  SKYETEK_STATUS st;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || 
      lpReader->lpDevice == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  if( (st = STR_GetSystemAddrForParm(parameter,&addr,lpReader->lpProtocol->version)) != SKYETEK_SUCCESS )
    return st;
  return lppi->GetDefaultSystemParameter(lpReader,&addr,lpData,SKYETEK_TIMEOUT);
}

SKYETEK_STATUS 
SkyeTekReader_SetDefaultSystemParameter(
    LPSKYETEK_READER             lpReader, 
    SKYETEK_SYSTEM_PARAMETER     parameter, 
    LPSKYETEK_DATA               lpData
    )
{
  LPPROTOCOLIMPL lppi;
  SKYETEK_ADDRESS addr;
  SKYETEK_STATUS st;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || 
      lpReader->lpDevice == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  if( (st = STR_GetSystemAddrForParm(parameter,&addr,lpReader->lpProtocol->version)) != SKYETEK_SUCCESS )
    return st;
  return lppi->SetDefaultSystemParameter(lpReader,&addr,lpData,SKYETEK_TIMEOUT);
}

SKYETEK_STATUS 
SkyeTekReader_AuthenticateReader(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_KEY        lpKey
    )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || 
      lpReader->lpDevice == NULL || lpKey == NULL || lpKey->lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->AuthenticateReader(lpReader,lpKey->lpData,SKYETEK_TIMEOUT);
}

SKYETEK_STATUS 
SkyeTekReader_EnableDebug(
    LPSKYETEK_READER     lpReader
    )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->EnableDebug(lpReader,SKYETEK_TIMEOUT);
}

SKYETEK_STATUS 
SkyeTekReader_DisableDebug(
    LPSKYETEK_READER     lpReader
    )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->DisableDebug(lpReader,SKYETEK_TIMEOUT);
}

SKYETEK_STATUS 
SkyeTekReader_GetDebugMessages(
  LPSKYETEK_READER     lpReader,
  LPSKYETEK_DATA      *lpData
  )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpProtocol->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->GetDebugMessages(lpReader, lpData,10000);
}

SKYETEK_STATUS 
SkyeTekReader_UploadFirmware(
    LPSKYETEK_READER                      lpReader, 
    TCHAR                                  *file, 
    unsigned char                         defaultsOnly,
    SKYETEK_FIRMWARE_UPLOAD_CALLBACK      callback, 
    void                                  *user
    )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->UploadFirmware(lpReader,file,defaultsOnly,callback,user);
}

int
SkyeTekReader_DoesRIDMatch(
      LPSKYETEK_READER      lpReader,
      const unsigned char   *id
    )
{
  unsigned long i;
  if( lpReader == NULL || lpReader->id == NULL || id == NULL )
    return 0;
  for( i = 0; i < lpReader->id->length; i++ )
  {
    if( lpReader->id->id[i] != id[i] )
      return 0;
  }
  return 1;
}

void
SkyeTekReader_CopyRIDToBuffer(
      LPSKYETEK_READER      lpReader,
      unsigned char   *id
    )
{
  unsigned long i;
  if( lpReader == NULL || lpReader->id == NULL || id == NULL )
    return;
  for( i = 0; i < lpReader->id->length; i++ )
  {
    id[i] = lpReader->id->id[i];
  }
}

SKYETEK_STATUS 
SkyeTekReader_EnterPaymentScanMode(
    LPSKYETEK_READER     lpReader
    )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || lpReader->lpDevice == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->EnterPaymentScanMode(lpReader, SKYETEK_TIMEOUT);
}

SKYETEK_API SKYETEK_STATUS 
SkyeTekReader_ScanPayments(
    LPSKYETEK_READER              lpReader,
    SKYETEK_PAYMENT_SCAN_CALLBACK callback,
    void                          *user
    )
{
  LPPROTOCOLIMPL lppi;
  if( lpReader == NULL || lpReader->lpProtocol == NULL || 
      lpReader->lpDevice == NULL || callback == NULL )
    return SKYETEK_INVALID_PARAMETER;
  lppi = (LPPROTOCOLIMPL)lpReader->lpProtocol->internal;
  return lppi->ScanPayments(lpReader, callback, user, SKYETEK_TIMEOUT);
}

READER_IMPL SkyetekReaderImpl = {
  SkyeTekReader_SelectTags,
  SkyeTekReader_GetTags,
  SkyeTekReader_GetTagsWithMask,
  SkyeTekReader_FreeTags,
  SkyeTekReader_StoreKey,
  SkyeTekReader_LoadKey,
  SkyeTekReader_LoadDefaults,
  SkyeTekReader_ResetDevice,
  SkyeTekReader_Bootload,
  SkyeTekReader_GetSystemParameter,
  SkyeTekReader_SetSystemParameter,
  SkyeTekReader_GetDefaultSystemParameter,
  SkyeTekReader_SetDefaultSystemParameter,
  SkyeTekReader_AuthenticateReader,
  SkyeTekReader_EnableDebug,
  SkyeTekReader_DisableDebug,
  SkyeTekReader_UploadFirmware,
  SkyeTekReader_GetDebugMessages,
  SkyeTekReader_DoesRIDMatch,
  SkyeTekReader_CopyRIDToBuffer,
  SkyeTekReader_EnterPaymentScanMode,
  SkyeTekReader_ScanPayments
};


