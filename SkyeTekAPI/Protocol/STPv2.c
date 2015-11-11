/**
 * STPv2.c
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implements the SkyeTek Protocol version 2 using
 * the new Transport abstraction layer.
 *
 * NOTE: STPv3 uses the request and response structs and
 *       the build and write request and read response 
 *       functions. This uses the older code for now that
 *       writes the requests and parses responses directly.
 *       This code can eventually be updated to the new
 *       request and response structs.
 */
#include "../SkyeTekAPI.h"
#include "../SkyeTekProtocol.h"
#include "../Device/Device.h"
#include "../Reader/Reader.h"
#include "../Tag/TagFactory.h"
#include "Protocol.h"
#include "CRC.h"
#include "STPv2.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#pragma warning(disable:4761)       // disable integral size mismatch warning

unsigned char genericV2ID[] = { 0xFF };
void SkyeTek_Debug( char * sz, ... );
void STP_DebugMsg(char *prefix, unsigned char *data, unsigned int len, unsigned char isASCII);

/*******************************************************************
 * API Functions
 *******************************************************************/

SKYETEK_API SKYETEK_STATUS STPV2_BuildRequest( LPSTPV2_REQUEST req)
{
  unsigned short crc_check;
  unsigned short ix = 0, iy = 0;
	int written = 0, totalWritten = 0;

  if( req == NULL )
    return SKYETEK_INVALID_PARAMETER;

  memset(req->msg,0,STPV2_MAX_ASCII_REQUEST_SIZE);

  /* Write ASCII message */
  if( req->isASCII )
  {
    req->msg[ix++] = STPV2_CR;
    req->msg[ix++] = crcGetASCIIFromHex(req->flags,1);
    req->msg[ix++] = crcGetASCIIFromHex(req->flags,0);
    req->msg[ix++] = crcGetASCIIFromHex(req->cmd,1);
    req->msg[ix++] = crcGetASCIIFromHex(req->cmd,0);
		if( req->flags & STPV2_RID )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->rid,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->rid,0);
		}
		if( req->tagType > 0 || req->cmd == STPV2_CMD_SELECT_TAG ||
			  req->cmd == STPV2_CMD_READ_TAG || req->cmd == STPV2_CMD_WRITE_TAG )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->tagType,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->tagType,0);
		}
		if( req->flags & STPV2_TID )
		{
			for( iy = 0; iy < 8; iy++ )
			{
				req->msg[ix++] = crcGetASCIIFromHex(req->tid[iy],1);
				req->msg[ix++] = crcGetASCIIFromHex(req->tid[iy],0);
			}
		}
		if( req->cmd == STPV2_CMD_SELECT_TAG && req->afiSession > 0 )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->afiSession,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->afiSession,0);
		}
		if( req->cmd != STPV2_CMD_SELECT_TAG )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->address,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->address,0);
			req->msg[ix++] = crcGetASCIIFromHex(req->numBlocks,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->numBlocks,0);
			if( req->cmd ==  STPV2_CMD_WRITE_TAG || req->cmd == STPV2_CMD_WRITE_SYSTEM || 
				req->cmd ==  STPV2_CMD_WRITE_MEMORY )
			{
				for( iy = 0; iy < req->dataLength; iy++ )
				{
					req->msg[ix++] = crcGetASCIIFromHex(req->data[iy],1);
					req->msg[ix++] = crcGetASCIIFromHex(req->data[iy],0);
				}
			}
		}

    /* Calculate CRC */
		if( req->flags & STPV2_CRC )
		{
			crc_check = crc16(0x0000, req->msg + 1, (ix-1));
			req->msg[ix++] = crcGetASCIIFromHex((crc_check >> 8),1);
			req->msg[ix++] = crcGetASCIIFromHex((crc_check >> 8),0);
			req->msg[ix++] = crcGetASCIIFromHex((crc_check & 0x00FF),1);
			req->msg[ix++] = crcGetASCIIFromHex((crc_check & 0x00FF),1);
		}
    req->msg[ix++] = STPV2_CR;
		req->msgLength = ix;
  }

  /* Write binary message */
  else
  {
    req->msg[0] = STPV2_STX;
    SkyeTek_Debug("STX:\t%02X\r\n",req->msg[0]);
    ix = 2;
    req->msg[ix++] = (unsigned char)req->flags | STPV2_CRC;
    SkyeTek_Debug("FLAGS:\t%02X ( ",req->msg[ix-1]);
    if( req->flags & STPV2_LOOP )
      SkyeTek_Debug("LOOP ");
    if( req->flags & STPV2_INV )
      SkyeTek_Debug("INV ");
    if( req->flags & STPV2_LOCK )
      SkyeTek_Debug("LOCK ");
    if( req->flags & STPV2_RF )
      SkyeTek_Debug("RF ");
    if( req->flags & STPV2_AFI )
      SkyeTek_Debug("AFI ");
    if( req->flags & STPV2_CRC )
      SkyeTek_Debug("CRC ");
    if( req->flags & STPV2_TID )
      SkyeTek_Debug("TID ");
    if( req->flags & STPV2_RID )
      SkyeTek_Debug("RID ");
    SkyeTek_Debug(")\r\n");

    req->msg[ix++] = (unsigned char)req->cmd;
    SkyeTek_Debug("CMD:\t%02X\r\n",req->msg[ix-1]);

    if( req->flags & STPV2_RID )
    {
			req->msg[ix++] = (unsigned char)req->rid;
      SkyeTek_Debug("RID:\t%02X\r\n",req->msg[ix-1]);
    }
		if( req->tagType > 0 || req->cmd == STPV2_CMD_SELECT_TAG ||
			  req->cmd == STPV2_CMD_READ_TAG || req->cmd == STPV2_CMD_WRITE_TAG )
    {
			req->msg[ix++] = (unsigned char)req->tagType;
      SkyeTek_Debug("TTYP:\t%02X\r\n",req->msg[ix-1]);
    }
		if( req->flags & STPV2_TID )
		{
      SkyeTek_Debug("TID:\t");
			for( iy = 0; iy < 8; iy++ )
      {
				req->msg[ix++] = req->tid[iy];
        SkyeTek_Debug("%02X",req->msg[ix-1]);
      }
      SkyeTek_Debug("\r\n");
		}
		if( req->cmd == STPV2_CMD_SELECT_TAG && req->afiSession > 0 )
    {
			req->msg[ix++] = (unsigned char)req->afiSession;
      SkyeTek_Debug("AFI:\t%02X\r\n",req->msg[ix-1]);
    }
		if( req->cmd != STPV2_CMD_SELECT_TAG )
		{
			req->msg[ix++] = (unsigned char)req->address;
      SkyeTek_Debug("ADDR:\t%02X\r\n",req->msg[ix-1]);
			//if( req->cmd != STPV2_CMD_READ_SYSTEM && req->cmd != STPV2_CMD_WRITE_SYSTEM )
		  req->msg[ix++] = (unsigned char)req->numBlocks;
      SkyeTek_Debug("BLKS:\t%02X\r\n",req->msg[ix-1]);
			if( req->cmd ==  STPV2_CMD_WRITE_TAG || req->cmd == STPV2_CMD_WRITE_SYSTEM || 
				req->cmd ==  STPV2_CMD_WRITE_MEMORY )
			{
        SkyeTek_Debug("DATA:\t");
				for( iy = 0; iy < req->dataLength; iy++ )
        {
					req->msg[ix++] = req->data[iy];
          SkyeTek_Debug("%02X",req->msg[ix-1]);
        }
        SkyeTek_Debug("\r\n");
			}
		}

		/* CRC required for binary */
		req->msg[1] = (unsigned char)ix;
    SkyeTek_Debug("LEN:\t%02X\r\n",req->msg[1]);
		crc_check = crc16(0x0000, req->msg + 1, (ix-1));
		req->msg[ix++] = crc_check >> 8;
		req->msg[ix++] = crc_check & 0x00FF;
    SkyeTek_Debug("CRC:\t%02X%02X\r\n",req->msg[ix-2],req->msg[ix-1]);
		req->msgLength = ix;
  }
  
  return SKYETEK_SUCCESS;
}

SKYETEK_API SKYETEK_STATUS STPV2_WriteRequest( 
  LPSKYETEK_DEVICE      device, 
  LPSTPV2_REQUEST       req, 
  unsigned int          timeout
  )
{
	unsigned int written = 0, totalWritten = 0;
	SKYETEK_STATUS status;
  LPDEVICEIMPL pd;

  if( device == NULL || req == NULL )
    return SKYETEK_INVALID_PARAMETER;
  
  pd = (LPDEVICEIMPL)device->internal;
  if( pd == NULL )
    return SKYETEK_INVALID_PARAMETER;

  if( (status = STPV2_BuildRequest(req)) != SKYETEK_SUCCESS )
		return status;
	
  STP_DebugMsg("request", req->msg, req->msgLength, req->isASCII);
	SkyeTek_Debug("code: %s\r\n", STPV2_LookupCommand(req->cmd));

	while( (written = pd->Write(device,req->msg+totalWritten,req->msgLength-totalWritten,timeout)) > 0 )
	{
		totalWritten += written;
		if( totalWritten >= req->msgLength )
			break;
	}
	if( written <= 0 )
		return SKYETEK_READER_IO_ERROR;
	
  return SKYETEK_SUCCESS;
}

SKYETEK_STATUS STPV2_ReadResponseImpl(
  LPSKYETEK_DEVICE    device, 
  LPSTPV2_REQUEST     req, 
  LPSTPV2_RESPONSE    resp, 
  unsigned int        timeout
  )
{
  unsigned short bytesRead = 0, totalRead = 0, ix = 0, iy = 0;
  unsigned char *ptr = NULL, *tmp = NULL;
  LPDEVICEIMPL pd;

	memset(resp,0,sizeof(STPV2_RESPONSE));

  if( device == NULL || resp == NULL )
    return SKYETEK_INVALID_PARAMETER;

  pd = (LPDEVICEIMPL)device->internal;
  if( pd == NULL )
    return SKYETEK_INVALID_PARAMETER;

	SkyeTek_Debug("timeout is: %d ms\r\n", (timeout+pd->timeout));

  if( req->isASCII )
	{
	  /* Read STX and message length */
	  bytesRead = pd->Read(device, resp->msg, 1, timeout);
	  if( bytesRead != 1 )
		  return SKYETEK_TIMEOUT;
  
		/* Check response */
		if( resp->msg[0] != STPV2_LF )
			return SKYETEK_READER_PROTOCOL_ERROR;

		/* Read in rest of message */
		SKYETEK_Sleep(100);
		ptr = resp->msg + 1;
		while((bytesRead = pd->Read(device, ptr, 1, timeout)))
		{
			totalRead += bytesRead;
			if( *ptr == STPV2_LF && *(ptr-1) == STPV2_CR )
				break;
			ptr += bytesRead;
		}

		/* Check for nothing to read */
		if( totalRead == 0 )
			return SKYETEK_TIMEOUT;

		resp->msgLength = totalRead + 1;

		/* Check size */
		if( resp->msgLength <= 1 || resp->msgLength > STPV2_MAX_ASCII_RESPONSE_SIZE )
    {
      resp->msgLength = 0;
			return SKYETEK_READER_PROTOCOL_ERROR;
    }

		/* Copy over */
		ix = 1;
		resp->code = crcGetHexFromASCII(&resp->msg[ix],2);
		ix += 2;

		/* Check for error code */
		if( STPV2_IsErrorResponse(resp->code) )
		{
			if( req->flags & STPV2_CRC && resp->msgLength > 3 ) 
			{
        ptr = resp->msg + 1;
				if(!verifyacrc(ptr, resp->msgLength-3, 1))
				{
					return SKYETEK_INVALID_CRC;
				}
			}
			return SKYETEK_SUCCESS;
		}

		if( req->flags & STPV2_RID ) 
		{
			resp->rid = crcGetHexFromASCII(&resp->msg[ix],2);
			ix += 2;
		}
		if( req->cmd == STPV2_CMD_SELECT_TAG && req->tagType == AUTO_DETECT )
		{
			resp->tagType = crcGetHexFromASCII(&resp->msg[ix],2);
			ix += 2;
		}
		if( resp->msgLength + 1 > ix )
		{
			resp->dataLength = totalRead - ix + 2;
      if( resp->dataLength > 2048 )
      {
        resp->dataLength = 0;
        return SKYETEK_READER_PROTOCOL_ERROR;
      }
			if( resp->dataLength > 0 )
			{
				for( iy = 0; iy < resp->dataLength; iy++ )
        {
					resp->data[iy] = resp->msg[ix++];
        }
			}
		}
		
		/* Adjust length to include CRs and LF */
		resp->msgLength += 2;

		/* Verify CRC */
		if( req->flags & STPV2_CRC && resp->msgLength > 3 ) 
		{
			if(!verifyacrc(resp->msg + 1, resp->msgLength-3, 1))
				return SKYETEK_INVALID_CRC;
		}
	}

	/* Binary mode */
	else
	{
	  /* Read STX and message length */
	  bytesRead = pd->Read(device, resp->msg, 2, timeout);
	  if( bytesRead != 2 )
    {
      SkyeTek_Debug("error: could not read 2 bytes: read %d bytes\r\n", bytesRead);
		  return SKYETEK_TIMEOUT;
    }
  
		/* Check response */
		if( resp->msg[0] != STPV2_STX )
		{
			STP_DebugMsg("response", resp->msg, 2, req->isASCII);
			if( resp->msg[0] == STPV2_NACK )
				return SKYETEK_READER_IN_BOOT_LOAD_MODE;
			else
				return SKYETEK_READER_PROTOCOL_ERROR;
		}

		/* Check message length */
		resp->msgLength = resp->msg[1];
		if( resp->msgLength > STPV2_MAX_ASCII_RESPONSE_SIZE )
    {
      resp->msgLength = 0;
			STP_DebugMsg("response", resp->msg, 2, req->isASCII);
			return SKYETEK_READER_PROTOCOL_ERROR;
    }

		/* Read in rest of message */
		SKYETEK_Sleep(100);
		ptr = resp->msg + 2;
		while((bytesRead = pd->Read(device, ptr, (resp->msgLength - totalRead), timeout)))
		{
			ptr += bytesRead;
			totalRead += bytesRead;
			SKYETEK_Sleep(20);
		}

		/* Copy over */
		ix = 2;
		resp->code = resp->msg[ix++];

		STP_DebugMsg("response", resp->msg, resp->msgLength, req->isASCII);
		SkyeTek_Debug("code: %s\r\n", STPV2_LookupResponse(resp->code));

		if( req->flags & STPV2_RID )
			resp->rid = resp->msg[ix++];
		if( req->cmd == STPV2_CMD_SELECT_TAG && req->tagType == AUTO_DETECT )
			resp->tagType = resp->msg[ix++];
		if( resp->msgLength + 1 > ix )
		{
			resp->dataLength = resp->msgLength - ix;
      if( resp->dataLength > 1024 )
      {
        resp->dataLength = 0;
        return SKYETEK_READER_PROTOCOL_ERROR;
      }
			if( resp->dataLength > 0 )
				for( iy = 0; iy < resp->dataLength; iy++ )
					resp->data[iy] = resp->msg[ix++];
		}

		/* Adjust length to include STX, length and CRC */
		resp->msgLength = ix + 2;

		/* Verify CRC */
		if(!verifycrc((resp->msg)+1, resp->msg[1], 0))
			return SKYETEK_INVALID_CRC;
	}

	return SKYETEK_SUCCESS;
} 

SKYETEK_API SKYETEK_STATUS STPV2_ReadResponse(
  LPSKYETEK_DEVICE    device, 
  LPSTPV2_REQUEST     req, 
  LPSTPV2_RESPONSE    resp,
  unsigned int        timeout
  )
{
  SKYETEK_STATUS st;
  unsigned int count = 0;
  
read:
  st = STPV2_ReadResponseImpl(device,req,resp,timeout);
  count++;
  if( resp->code == 0 || resp->code & 0x80 )
  {
    return st;
  }
  else if( req->cmd == STPV2_CMD_SELECT_TAG &&
           resp->code == STPV2_RESP_SELECT_TAG_LOOP_ON )
  {
    return st;
  }
  else if( (resp->code & 0x7F) == (req->cmd & 0x7F) )
  {
    return st;
  }
  else if( req->anyResponse )
  {
    return st;
  }
  else
  {
		SkyeTek_Debug("error: response code 0x%X doesn't match request 0x%X\r\n", resp->code, req->cmd);
    count++;
    if( count > 10 )
      return st;
    goto read;
  }
}

SKYETEK_STATUS STPV2_GetStatus(unsigned char code)
{
	int ix;
	for( ix = 0; ix < STPV2_STATUS_LOOKUPS_COUNT; ix++ )
	{
		if( stpv2Statuses[ix].cmd == code )
			return stpv2Statuses[ix].status;
	}
	return SKYETEK_FAILURE;
}


SKYETEK_STATUS 
STPV2_SendCommand(
    LPSKYETEK_READER     lpReader,
    unsigned char         cmd,
    unsigned char         extraFlags,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;

  if(lpReader == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Send request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;

  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }
  
writeCommand:
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 

}

SKYETEK_STATUS 
STPV2_SendSetCommand(
    LPSKYETEK_READER     lpReader,
    unsigned char        cmd,
    unsigned char        extraFlags,
    LPSKYETEK_DATA       lpData,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
	unsigned int iy = 0;
  LPREADER_IMPL lpri;

  if(lpReader == NULL || lpData == NULL || lpData->data == NULL || lpData->size > 1024 ||
    lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.dataLength = lpData->size;
	for(iy = 0; iy < req.dataLength && iy < 1024; iy++)
		req.data[iy] = lpData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 

}

SKYETEK_STATUS 
STPV2_SendAddrGetCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       *lpData,
    unsigned int          timeout
    )
{
  STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;

  if(lpReader == NULL || lpAddr == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.address = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks & 0x00FF; 
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
  if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
  
  *lpData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpData,resp.data,resp.dataLength);
}

SKYETEK_STATUS 
STPV2_SendAddrSetCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpData,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpAddr == NULL || lpData == NULL ||
    lpData->data == NULL || lpData->size > 1024 )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.address = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks & 0x00FF; 
	req.dataLength = lpData->size;
	for(iy = 0; iy < req.dataLength && iy < 1024; iy++)
		req.data[iy] = lpData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 
}

SKYETEK_STATUS 
STPV2_SendTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned char         cmd,
    unsigned char         extraFlags,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.tagType = lpTag->type & 0x00FF;
	if( (req.flags & STPV2_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    for(iy = 0; iy < lpTag->id->length && iy < 8; iy++)
      req.tid[iy] = lpTag->id->id[iy];
	}
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	/* Send request */
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	SKYETEK_Sleep(100);
  

	/* Read response */
	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
  
  return SKYETEK_SUCCESS;	
}

SKYETEK_STATUS 
STPV2_SendGetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       *lpData,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.tagType = lpTag->type & 0x00FF;
	if( (req.flags & STPV2_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    for( iy = (lpTag->id->length < 8 ? lpTag->id->length-1 : 7); iy >= 0; iy-- )
      req.tid[iy] = lpTag->id->id[iy];
	}
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
  
  *lpData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpData,resp.data,resp.dataLength);

}

SKYETEK_STATUS 
STPV2_SendSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpData,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpData == NULL ||
    lpData->data == NULL || lpData->size > 1024 )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV2_REQUEST));
  req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.tagType = lpTag->type & 0x00FF;
	if( (req.flags & STPV2_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    for( iy = (lpTag->id->length < 8 ? lpTag->id->length-1 : 7); iy >= 0; iy-- )
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.dataLength = lpData->size;
	for(iy = 0; iy < lpData->size && iy < 1024; iy++)
		req.data[iy] = lpData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 

}

SKYETEK_STATUS 
STPV2_SendGetSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpSendData == NULL ||
    lpSendData->data == NULL || lpSendData->size > 1024 || lpRecvData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV2_REQUEST));
  req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.tagType = lpTag->type & 0x00FF;
	if( (req.flags & STPV2_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    for( iy = (lpTag->id->length < 8 ? lpTag->id->length-1 : 7); iy >= 0; iy-- )
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.dataLength = lpSendData->size;
	for(iy = 0; iy < lpSendData->size && iy < 1024; iy++)
		req.data[iy] = lpSendData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
    
  *lpRecvData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpRecvData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpRecvData,resp.data,resp.dataLength);
}

SKYETEK_STATUS 
STPV2_SendAddrGetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       *lpData,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpAddr == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.tagType = lpTag->type & 0x00FF;
	if( (req.flags & STPV2_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    for( iy = (lpTag->id->length < 8 ? lpTag->id->length-1 : 7); iy >= 0; iy-- )
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.address = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks & 0x00FF;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
  
  *lpData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpData,resp.data,resp.dataLength);

}

SKYETEK_STATUS 
STPV2_SendAddrSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpData,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpAddr == NULL || 
      lpData == NULL || lpData->size > 1024 )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.tagType = lpTag->type & 0x00FF;
	if( (req.flags & STPV2_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    for( iy = (lpTag->id->length < 8 ? lpTag->id->length-1 : 7); iy >= 0 && iy < 8; iy-- )
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.address = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks & 0x00FF;
	req.dataLength = lpData->size;
	for(iy = 0; iy < lpData->size && iy < 1024; iy++)
		req.data[iy] = lpData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	/* Send request */
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	SKYETEK_Sleep(100);
  
  /* Read response */
	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
  
  return SKYETEK_SUCCESS;	
}

SKYETEK_STATUS 
STPV2_SendAddrGetSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int          timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpSendData == NULL ||
    lpSendData->data == NULL || lpSendData->size > 1024 || lpRecvData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV2_REQUEST));
  req.cmd = cmd;
	req.flags = STPV2_CRC;
  req.flags |= extraFlags;
	req.tagType = lpTag->type & 0x00FF;
	if( (req.flags & STPV2_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    for( iy = (lpTag->id->length < 8 ? lpTag->id->length-1 : 7); iy >= 0; iy-- )
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.address = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks & 0x00FF;
	req.dataLength = lpSendData->size;
	for(iy = 0; iy < lpSendData->size && iy < 1024; iy++)
		req.data[iy] = lpSendData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

writeCommand:
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV2_GetStatus(resp.code);
    
  *lpRecvData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpRecvData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpRecvData,resp.data,resp.dataLength);
}


/* Reader Functions */
SKYETEK_STATUS 
STPV2_StopSelectLoop(
  LPSKYETEK_READER   lpReader,
  unsigned int       timeout
  )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	int ix = 0, iy = 0;

  if(lpReader == NULL)
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = STPV2_CMD_SELECT_TAG;
	req.flags = STPV2_CRC;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

	/* Send request */
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	/* Read response */
	SKYETEK_Sleep(100);
	memset(&resp,0,sizeof(STPV2_RESPONSE));
	return STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
}

SKYETEK_STATUS 
STPV2_SelectTags(
  LPSKYETEK_READER             lpReader, 
  SKYETEK_TAGTYPE              tagType, 
  PROTOCOL_TAG_SELECT_CALLBACK callback,
  PROTOCOL_FLAGS               flags,
  void                         *user,
  unsigned int                 timeout
  )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPSKYETEK_DATA lpd;
  LPREADER_IMPL lpri;
	int ix = 0, iy = 0;

  if((lpReader == NULL) || (callback == 0))
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = STPV2_CMD_SELECT_TAG;
	req.flags = STPV2_CRC | ((flags.isInventory == 1) ? STPV2_INV : 0);
  req.flags = req.flags | ((flags.isLoop == 1) ? STPV2_LOOP : 0);
	req.tagType = (unsigned char) tagType;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

	/* Send request */
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	SKYETEK_Sleep(100);

readResponse:
	/* Read response */
	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
  if( status == SKYETEK_TIMEOUT )
  {
    if(!callback(tagType, NULL, user))
    {
      STPV2_StopSelectLoop(lpReader,timeout);
      return SKYETEK_SUCCESS;
    }
    goto readResponse;
  }
	if( status != SKYETEK_SUCCESS )
		return status;

	if( resp.code == STPV2_RESP_SELECT_TAG_LOOP_ON )
		goto readResponse;

	if( resp.code == STPV2_RESP_SELECT_TAG_FAIL || resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF )
    return SKYETEK_SUCCESS;

	if( resp.code == STPV2_RESP_SELECT_TAG_PASS )
	{
		/* Copy over tag information */
    if( resp.tagType != 0 )
      tagType = (SKYETEK_TAGTYPE)resp.tagType;
    else
      tagType = (SKYETEK_TAGTYPE)req.tagType;

    lpd = SkyeTek_AllocateData(resp.dataLength);
    SkyeTek_CopyBuffer(lpd,resp.data,resp.dataLength);
  
		/* Call the callback */
		if(!callback(tagType, lpd, user))
		{
      SkyeTek_FreeData(lpd);
			STPV2_StopSelectLoop(lpReader,timeout);
      return SKYETEK_SUCCESS;
		}
    SkyeTek_FreeData(lpd);

		/* Check for bail */
		if(!flags.isInventory && !flags.isLoop)
      return SKYETEK_SUCCESS;
		
		/* Keep reading */
		goto readResponse;
	}
  return status;
}

SKYETEK_STATUS 
STPV2_GetTags(
  LPSKYETEK_READER   lpReader, 
  SKYETEK_TAGTYPE    tagType, 
  LPTAGTYPE_ARRAY    **tagTypes, 
  LPSKYETEK_DATA     **lpData, 
  unsigned int       *count,
  unsigned int       timeout
  )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int ix = 0, iy = 0, num = 0, step = 5;

  if(lpReader == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = STPV2_CMD_SELECT_TAG;
	req.flags = STPV2_CRC | STPV2_INV;
	req.tagType = tagType;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericV2ID) )
  {
    lpri->CopyRIDToBuffer(lpReader,&req.rid);
    req.flags |= STPV2_RID;
  }

	/* Send request */
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		goto failure;
	SKYETEK_Sleep(100);
  
readResponse:
	/* Read response */
	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		goto success; /* done reading */

	if( resp.code == STPV2_RESP_SELECT_TAG_FAIL )
		goto success;

	if( resp.code == STPV2_RESP_SELECT_TAG_PASS )
	{
		/* Allocate memory */
		if( !(num % step) )
    {
			*tagTypes = (LPTAGTYPE_ARRAY*)realloc(*tagTypes, (num + step) * sizeof(LPTAGTYPE_ARRAY));
			*lpData = (LPSKYETEK_DATA*)realloc(*lpData, (num + step) * sizeof(LPSKYETEK_DATA));
    }
		if( *tagTypes == NULL || lpData == NULL )
		{
			status = SKYETEK_OUT_OF_MEMORY;
			goto failure;
		}
    (*tagTypes)[num] = (LPTAGTYPE_ARRAY)malloc(sizeof(TAGTYPE_ARRAY));
    if( resp.tagType != 0 )
      (*tagTypes)[num]->type = (SKYETEK_TAGTYPE)resp.tagType;
    else
      (*tagTypes)[num]->type = (SKYETEK_TAGTYPE)req.tagType;
		(*lpData)[num] = SkyeTek_AllocateData(resp.dataLength);
		if( (*lpData)[num] == NULL )
		{
			status = SKYETEK_OUT_OF_MEMORY;
			goto failure;
		}
    SkyeTek_CopyBuffer((*lpData)[num],resp.data,resp.dataLength);
		num++;

		/* Keep reading */
		goto readResponse;
	}
 
failure:
  if( *tagTypes != NULL )
    free(*tagTypes);
  if( *lpData != NULL )
  {
    for( ix = 0; ix < num; ix++ )
      SkyeTek_FreeData((*lpData)[ix]);
    free(*lpData);
  }
	*count = 0;
	return status;

success:
	*count = num;
  return SKYETEK_SUCCESS;
}


SKYETEK_STATUS 
STPV2_GetTagsWithMask(
  LPSKYETEK_READER   lpReader, 
  SKYETEK_TAGTYPE    tagType, 
  LPSKYETEK_ID	     lpTagIdMask,
  LPTAGTYPE_ARRAY    **tagTypes, 
  LPSKYETEK_DATA     **lpData, 
  unsigned int       *count,
  unsigned int       timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_StoreKey(
  LPSKYETEK_READER     lpReader,
  SKYETEK_TAGTYPE      type,
  LPSKYETEK_ADDRESS    lpAddr,
  LPSKYETEK_DATA       lpData,
  unsigned int       timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_LoadKey(
  LPSKYETEK_READER     lpReader, 
  LPSKYETEK_ADDRESS    lpAddr,
  unsigned int       timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}


SKYETEK_STATUS 
STPV2_LoadDefaults(
  LPSKYETEK_READER     lpReader,
  unsigned int       timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ResetDevice(
  LPSKYETEK_READER     lpReader,
  unsigned int       timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_Bootload(
  LPSKYETEK_READER     lpReader,
  unsigned int       timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_GetSystemParameter(
  LPSKYETEK_READER             lpReader, 
  LPSKYETEK_ADDRESS            lpAddr,
  LPSKYETEK_DATA               *lpData,
  unsigned int                 timeout
  )
{
  return STPV2_SendAddrGetCommand(lpReader,lpAddr,STPV2_CMD_READ_SYSTEM,0,lpData,timeout);
}

SKYETEK_STATUS 
STPV2_SetSystemParameter(
  LPSKYETEK_READER             lpReader, 
  LPSKYETEK_ADDRESS            lpAddr,
  LPSKYETEK_DATA               lpData,
  unsigned int                 timeout
  )
{
  return STPV2_SendAddrSetCommand(lpReader,lpAddr,STPV2_CMD_WRITE_SYSTEM,0,lpData,timeout);
}

SKYETEK_STATUS 
STPV2_GetDefaultSystemParameter(
  LPSKYETEK_READER             lpReader, 
  LPSKYETEK_ADDRESS            lpAddr,
  LPSKYETEK_DATA               *lpData,
  unsigned int                 timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_SetDefaultSystemParameter(
  LPSKYETEK_READER             lpReader, 
  LPSKYETEK_ADDRESS            lpAddr,
  LPSKYETEK_DATA               lpData,
  unsigned int                 timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_AuthenticateReader(
  LPSKYETEK_READER     lpReader, 
  LPSKYETEK_DATA       lpData,
  unsigned int         timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_EnableDebug(
  LPSKYETEK_READER     lpReader,
  unsigned int         timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_DisableDebug(
  LPSKYETEK_READER     lpReader,
  unsigned int         timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_GetDebugMessages(
  LPSKYETEK_READER     lpReader,
  LPSKYETEK_DATA       *lpData,
  unsigned int         timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_UploadFirmware(
    LPSKYETEK_READER                      lpReader,
    TCHAR                                  *file, 
    unsigned char                         defaultsOnly,
    SKYETEK_FIRMWARE_UPLOAD_CALLBACK      callback, 
    void                                  *user
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_EnterPaymentScanMode(
  LPSKYETEK_READER     lpReader,
  unsigned int timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ScanPayments(
  LPSKYETEK_READER              lpReader,
  SKYETEK_PAYMENT_SCAN_CALLBACK callback,
  void                          *user,
  unsigned int                  timeout
  )
{
  return SKYETEK_NOT_SUPPORTED;
}

/* Tag functions */
SKYETEK_STATUS 
STPV2_SelectTag(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
	STPV2_REQUEST req;
	STPV2_RESPONSE resp;
	SKYETEK_STATUS status;
	int ix = 0, iy = 0;
	TCHAR *str = NULL;

  if(lpReader == NULL || lpTag == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Build request */
	memset(&req,0,sizeof(STPV2_REQUEST));
	req.cmd = STPV2_CMD_SELECT_TAG;
	req.flags = STPV2_CRC;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    req.flags |= STPV2_TID;
  req.flags |= (lpTag->rf > 0 ? STPV2_RF : 0);
	req.flags |= (lpTag->afi > 0 ? STPV2_AFI : 0);
	req.afiSession = (unsigned char)lpTag->afi;
	req.tagType = (unsigned char)lpTag->type;
	if( lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    for( iy = (lpTag->id->length < 8 ? lpTag->id->length-1 : 7); iy >= 0; iy-- )
      req.tid[iy] = lpTag->id->id[iy];
	}

writeCommand:

	/* Send request */
	status = STPV2_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  SKYETEK_Sleep(100);


	/* Read response */
	memset(&resp,0,sizeof(STPV2_RESPONSE));
	status = STPV2_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
	/* Process response */
  if(resp.code == STPV2_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != STPV2_RESP_SELECT_TAG_PASS)
    return STPV2_GetStatus(resp.code);
  
  if(lpTag->type == AUTO_DETECT)
  {
    lpTag->type = (SKYETEK_TAGTYPE)resp.tagType;
    InitializeTagType(lpTag);
  }
  
  /* If TID Flag is not set, then populate the tag ID into the tag buffer */
  if(!(req.flags & STPV2_TID))
  {
    if( lpTag->id != NULL )
      SkyeTek_FreeID(lpTag->id);
    lpTag->id = SkyeTek_AllocateID(resp.dataLength);
    if( lpTag->id == NULL )
      return SKYETEK_OUT_OF_MEMORY;
    status = SkyeTek_CopyBuffer((LPSKYETEK_DATA)lpTag->id, resp.data, resp.dataLength);
    if( status != SKYETEK_SUCCESS )
      return status;
    str = SkyeTek_GetStringFromID(lpTag->id);
    if( str != NULL )
    {
      _tcscpy(lpTag->friendly,str);
      SkyeTek_FreeString(str);
    }
  }
    
  return SKYETEK_SUCCESS; 
}

SKYETEK_STATUS 
STPV2_ReadTagData(    
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    PROTOCOL_FLAGS       flags,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  unsigned char f = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    f |= STPV2_TID;
  f |= (lpTag->rf > 0 ? STPV2_RF : 0);
  return STPV2_SendAddrGetTagCommand(lpReader,lpTag,lpAddr,STPV2_CMD_READ_TAG,f,lpData,timeout);
}

SKYETEK_STATUS 
STPV2_WriteTagData(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    PROTOCOL_FLAGS       flags,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned char extra = 0;
  if( flags.isLock )
    extra |= STPV2_LOCK;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    extra |= STPV2_TID;
  extra |= (lpTag->rf > 0 ? STPV2_RF : 0);
  return STPV2_SendAddrSetTagCommand(lpReader,lpTag,lpAddr,STPV2_CMD_WRITE_TAG,extra,lpData,timeout);
}

SKYETEK_STATUS 
STPV2_ReadTagConfig(    
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    PROTOCOL_FLAGS       flags,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_WriteTagConfig(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    PROTOCOL_FLAGS       flags,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ActivateTagType(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_DeactivateTagType(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_SetTagBitRate(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_GetTagInfo(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_GetLockStatus(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_KillTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ReviveTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_EraseTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag, 
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_FormatTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_DeselectTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_AuthenticateTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_SendTagPassword(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_GetApplicationIDs(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_SelectApplication(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_CreateApplication(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_DeleteApplication(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_GetFileIDs(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_SelectFile(
    LPSKYETEK_READER    lpReader,
    LPSKYETEK_TAG       lpTag,
    LPSKYETEK_DATA      lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_CreateFile(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
} 

SKYETEK_STATUS 
STPV2_GetFileSettings(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ChangeFileSettings(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ReadFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_WriteFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_DeleteFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ClearFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_CreditValueFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}


SKYETEK_STATUS 
STPV2_DebitValueFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_LimitedCreditValueFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_GetValue(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ReadRecords(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}
    	
SKYETEK_STATUS 
STPV2_WriteRecord(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_CommitTransaction(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_AbortTransaction(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_EnableEAS(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_DisableEAS(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ScanEAS(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ReadAFI(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}
    	
SKYETEK_STATUS 
STPV2_WriteAFI(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ReadDSFID(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}
    	
SKYETEK_STATUS 
STPV2_WriteDSFID(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_GetKeyVersion(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ChangeKey(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_GetKeySettings(
    LPSKYETEK_READER        lpReader,
    LPSKYETEK_TAG           lpTag,
    LPSKYETEK_DATA          *lpRecvData,
    unsigned int            timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ChangeKeySettings(
    LPSKYETEK_READER             lpReader,
    LPSKYETEK_TAG                lpTag,
    LPSKYETEK_DATA               lpData,
    unsigned int                 timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_InitializeSecureMemoryTag(
    LPSKYETEK_READER          lpReader,
    LPSKYETEK_TAG             lpTag,
    LPSKYETEK_DATA            lpData,
    unsigned int              timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_SetupSecureMemoryTag(
    LPSKYETEK_READER          lpReader,
    LPSKYETEK_TAG             lpTag,
    LPSKYETEK_DATA            lpData,
    unsigned int              timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_InterfaceSend(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_TransportSend(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_InitiatePayment(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV2_ComputePayment(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

unsigned char STPV2_IsErrorResponse(unsigned short code)
{
	return( ((code & 0xF0) == 0x80) || ((code & 0xF0) == 0x90));
}

SKYETEK_API TCHAR *STPV2_LookupCommand( unsigned char cmd )
{
	int ix;
	for( ix = 0; ix < STPV2_CMD_LOOKUPS_COUNT; ix++ )
	{
		if( stpv2Commands[ix].cmd == cmd )
			return stpv2Commands[ix].name;
	}
	return _T("Unknown");
}

SKYETEK_API unsigned char STPV2_LookupCommandCode( TCHAR *cmdName )
{
	int ix;
	for( ix = 0; ix < STPV2_CMD_LOOKUPS_COUNT; ix++ )
	{
		if( _tcscmp(stpv2Commands[ix].name, cmdName) == 0 )
			return stpv2Commands[ix].cmd;
	}
	return 0;
}

SKYETEK_API int STPV2_GetCommandCount()
{
	return STPV2_CMD_LOOKUPS_COUNT;
}

SKYETEK_API TCHAR *STPV2_GetCommandNameAt( int ix )
{
	if( ix >= 0 && ix < STPV2_CMD_LOOKUPS_COUNT )
		return stpv2Commands[ix].name;
	return _T("Unknown");
}

SKYETEK_API TCHAR *STPV2_LookupResponse( unsigned char resp )
{
	int ix;
	for( ix = 0; ix < STPV2_RESP_LOOKUPS_COUNT; ix++ )
	{
		if( stpv2Responses[ix].cmd == resp )
			return stpv2Responses[ix].name;
	}
	return _T("Unknown");
}

unsigned char STPV2_LookupResponseCode( TCHAR *respName )
{
	int ix;
	for( ix = 0; ix < STPV2_RESP_LOOKUPS_COUNT; ix++ )
	{
		if( _tcscmp(stpv2Responses[ix].name, respName) == 0 )
			return stpv2Responses[ix].cmd;
	}
	return 0;
}

int STPV2_GetResponsesCount()
{
	return STPV2_RESP_LOOKUPS_COUNT;
}

TCHAR *STPV2_GetResponseNameAt( int ix )
{
	if( ix >= 0 && ix < STPV2_RESP_LOOKUPS_COUNT )
		return stpv2Responses[ix].name;
	return _T("Unknown");
}

PROTOCOLIMPL STPV2Impl = {
  2,
  STPV2_SelectTags,
  STPV2_GetTags,
  STPV2_GetTagsWithMask,
  STPV2_StoreKey,
  STPV2_LoadKey,
  STPV2_LoadDefaults,
  STPV2_ResetDevice,
  STPV2_Bootload,
  STPV2_GetSystemParameter,
  STPV2_SetSystemParameter,
  STPV2_GetDefaultSystemParameter,
  STPV2_SetDefaultSystemParameter,
  STPV2_AuthenticateReader,
  STPV2_EnableDebug,
  STPV2_DisableDebug,
  STPV2_UploadFirmware,
  STPV2_EnterPaymentScanMode,
  STPV2_ScanPayments,
  STPV2_SelectTag,
  STPV2_ReadTagData,    
  STPV2_WriteTagData,
  STPV2_ReadTagConfig,    
  STPV2_WriteTagConfig,
  STPV2_ActivateTagType,
  STPV2_DeactivateTagType,
  STPV2_SetTagBitRate,
  STPV2_GetTagInfo,
  STPV2_GetLockStatus,
  STPV2_KillTag,
  STPV2_ReviveTag,
  STPV2_EraseTag,
  STPV2_FormatTag,
  STPV2_DeselectTag,
  STPV2_AuthenticateTag,
  STPV2_SendTagPassword,
  STPV2_GetApplicationIDs,
  STPV2_SelectApplication,
  STPV2_CreateApplication,
  STPV2_DeleteApplication,
  STPV2_GetFileIDs,
  STPV2_SelectFile,
  STPV2_CreateFile,
  STPV2_GetFileSettings,
  STPV2_ChangeFileSettings,
  STPV2_ReadFile,
  STPV2_WriteFile,
  STPV2_DeleteFile,
  STPV2_ClearFile,
  STPV2_CreditValueFile,
  STPV2_DebitValueFile,
  STPV2_LimitedCreditValueFile,
  STPV2_GetValue,
  STPV2_ReadRecords,
  STPV2_WriteRecord,
  STPV2_CommitTransaction,
  STPV2_AbortTransaction,
  STPV2_EnableEAS,
  STPV2_DisableEAS,
  STPV2_ScanEAS,
  STPV2_ReadAFI,
  STPV2_WriteAFI,
  STPV2_ReadDSFID,
  STPV2_WriteDSFID,
  STPV2_GetKeyVersion,
  STPV2_ChangeKey,
  STPV2_GetKeySettings,
  STPV2_ChangeKeySettings,
  STPV2_InitializeSecureMemoryTag,
  STPV2_SetupSecureMemoryTag,
  STPV2_InterfaceSend,
  STPV2_TransportSend,
  STPV2_InitiatePayment,
  STPV2_ComputePayment,
  STPV2_GetDebugMessages
};
