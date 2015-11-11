/**
 * STPv3.c
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implements the SkyeTek Protocol version 2 using
 * the new Transport abstraction layer.
 */
#include "../SkyeTekAPI.h"
#include "../SkyeTekProtocol.h"
#include "../Device/Device.h"
#include "../Reader/Reader.h"
#include "../Tag/TagFactory.h"
#include "Protocol.h"
#include "CRC.h"
#include "STPv3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef WINCE
#include <sys/types.h>
#include <sys/stat.h>
#endif

#pragma warning(disable:4761)       // disable integral size mismatch warning

unsigned char genericID[] = { 0xFF, 0xFF, 0xFF, 0xFF };

void SkyeTek_Debug( TCHAR * sz, ... );

void STP_DebugMsg(TCHAR *prefix, unsigned char *data, unsigned int len, unsigned char isASCII)
{
	unsigned int i, j;
	unsigned int size = len * 2 + 1;
	TCHAR *msg = (TCHAR *)malloc(size*sizeof(TCHAR));
	TCHAR *ptr = msg;
	memset(msg,0,size*sizeof(TCHAR));

	for( i = 0, j = 0; i < len && j < size; i++, j++ )
	{
		if( isASCII )
		{
			msg[j] = data[i];
		}
		else
		{
      _stprintf(&msg[j],_T("%02X"),data[i]);
      j++;
		}
	}
	msg[j] = _T('\0');
	SkyeTek_Debug(_T("%s: %s\r\n"), prefix, msg);
	free(msg);
}

SKYETEK_API SKYETEK_STATUS STPV3_BuildRequest( LPSTPV3_REQUEST req)
{
  unsigned short crc_check;
  unsigned int ix = 0, iy = 0, state = 0, len = 0;
	int written = 0, totalWritten = 0;

  if( req == NULL )
    return SKYETEK_INVALID_PARAMETER;

  memset(req->msg,0,STPV3_MAX_ASCII_REQUEST_SIZE);

  /* Write ASCII message */
  if( req->isASCII )
  {
    req->msg[ix++] = STPV3_CR;
    req->msg[ix++] = crcGetASCIIFromHex(req->flags,3);
    req->msg[ix++] = crcGetASCIIFromHex(req->flags,2);
    req->msg[ix++] = crcGetASCIIFromHex(req->flags,1);
    req->msg[ix++] = crcGetASCIIFromHex(req->flags,0);
    req->msg[ix++] = crcGetASCIIFromHex(req->cmd,3);
    req->msg[ix++] = crcGetASCIIFromHex(req->cmd,2);
    req->msg[ix++] = crcGetASCIIFromHex(req->cmd,1);
    req->msg[ix++] = crcGetASCIIFromHex(req->cmd,0);
		if( req->flags & STPV3_RID )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->rid[0],1);
			req->msg[ix++] = crcGetASCIIFromHex(req->rid[0],0);
			req->msg[ix++] = crcGetASCIIFromHex(req->rid[1],1);
			req->msg[ix++] = crcGetASCIIFromHex(req->rid[1],0);
			req->msg[ix++] = crcGetASCIIFromHex(req->rid[2],1);
			req->msg[ix++] = crcGetASCIIFromHex(req->rid[2],0);
			req->msg[ix++] = crcGetASCIIFromHex(req->rid[3],1);
			req->msg[ix++] = crcGetASCIIFromHex(req->rid[3],0);
		}
		if( (req->cmd >> 8) == 0x01 || (req->cmd >> 8) == 0x02 ||
        (req->cmd >> 8) == 0x03 || (req->cmd >> 8) == 0x04 ||
        (req->cmd >> 8) == 0x05 || (req->cmd >> 8) == 0x06 )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->tagType,3);
			req->msg[ix++] = crcGetASCIIFromHex(req->tagType,2);
			req->msg[ix++] = crcGetASCIIFromHex(req->tagType,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->tagType,0);
		}
		if( req->flags & STPV3_TID )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->tidLength,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->tidLength,0);
			for( iy = 0; iy < req->tidLength; iy++ )
			{
				req->msg[ix++] = crcGetASCIIFromHex(req->tid[iy],1);
				req->msg[ix++] = crcGetASCIIFromHex(req->tid[iy],0);
			}
		}
		if( req->flags & STPV3_AFI )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->afi,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->afi,0);
		}
		if( req->flags & STPV3_SESSION )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->session,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->session,0);
		}
		state = STPV3_IsAddressOrDataCommand(req->cmd);
		if( state & STPV3_FORMAT_ADDRESS )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->address[0],1);
			req->msg[ix++] = crcGetASCIIFromHex(req->address[0],0);
			req->msg[ix++] = crcGetASCIIFromHex(req->address[1],1);
			req->msg[ix++] = crcGetASCIIFromHex(req->address[1],0);
		}
		if( state & STPV3_FORMAT_BLOCKS )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->numBlocks,3);
			req->msg[ix++] = crcGetASCIIFromHex(req->numBlocks,2);
			req->msg[ix++] = crcGetASCIIFromHex(req->numBlocks,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->numBlocks,0);
		}
		if( req->flags & STPV3_DATA )
		{
			req->msg[ix++] = crcGetASCIIFromHex(req->dataLength,3);
			req->msg[ix++] = crcGetASCIIFromHex(req->dataLength,2);
			req->msg[ix++] = crcGetASCIIFromHex(req->dataLength,1);
			req->msg[ix++] = crcGetASCIIFromHex(req->dataLength,0);
			for( iy = 0; iy < req->dataLength; iy++ )
			{
				req->msg[ix++] = crcGetASCIIFromHex(req->data[iy],1);
				req->msg[ix++] = crcGetASCIIFromHex(req->data[iy],0);
			}
		}

    /* Calculate CRC */
		if( req->flags & STPV3_CRC )
		{
			crc_check = crca16(0x0000, (req->msg)+1, (ix-1));
			req->msg[ix++] = crcGetASCIIFromHex((crc_check >> 8),1);
			req->msg[ix++] = crcGetASCIIFromHex((crc_check >> 8),0);
			req->msg[ix++] = crcGetASCIIFromHex((crc_check & 0x00FF),1);
			req->msg[ix++] = crcGetASCIIFromHex((crc_check & 0x00FF),0);
		}
    req->msg[ix++] = STPV3_CR;
	  req->msgLength = ix;
  }

  /* Write binary message */
  else
  {
    req->msg[0] = STPV3_STX;
    SkyeTek_Debug(_T("STX:\t%02X\r\n"),req->msg[0]);
    ix = 3;
    req->msg[ix++] = req->flags >> 8;
    req->msg[ix++] = req->flags & 0x00FF;
    
    SkyeTek_Debug(_T("FLAGS:\t%02X%02X ( "),req->msg[ix-2],req->msg[ix-1]);
    if( req->flags & STPV3_LOOP )
      SkyeTek_Debug(_T("LOOP "));
    if( req->flags & STPV3_INV )
      SkyeTek_Debug(_T("INV "));
    if( req->flags & STPV3_LOCK )
      SkyeTek_Debug(_T("LOCK "));
    if( req->flags & STPV3_RF )
      SkyeTek_Debug(_T("RF "));
    if( req->flags & STPV3_AFI )
      SkyeTek_Debug(_T("AFI "));
    if( req->flags & STPV3_CRC )
      SkyeTek_Debug(_T("CRC "));
    if( req->flags & STPV3_TID )
      SkyeTek_Debug(_T("TID "));
    if( req->flags & STPV3_RID )
      SkyeTek_Debug(_T("RID "));
    if( req->flags & STPV3_ENCRYPTION )
      SkyeTek_Debug(_T("ENC "));
    if( req->flags & STPV3_HMAC )
      SkyeTek_Debug(_T("HMAC "));
    if( req->flags & STPV3_SESSION )
      SkyeTek_Debug(_T("SESS "));
    if( req->flags & STPV3_DATA )
      SkyeTek_Debug(_T("DATA "));
    SkyeTek_Debug(_T(")\r\n"));


    req->msg[ix++] = req->cmd >> 8;
    req->msg[ix++] = req->cmd & 0x00FF;
    SkyeTek_Debug(_T("CMD:\t%02X%02X\r\n"),req->msg[ix-2],req->msg[ix-1]);
		if( req->flags & STPV3_RID )
		{
			req->msg[ix++] = req->rid[0];
			req->msg[ix++] = req->rid[1];
			req->msg[ix++] = req->rid[2];
			req->msg[ix++] = req->rid[3];
      SkyeTek_Debug(_T("RID:\t%02X%02X%02X%02X\r\n"),req->msg[ix-4],req->msg[ix-3],req->msg[ix-2],req->msg[ix-1]);
		}
		if( (req->cmd >> 8) == 0x01 || (req->cmd >> 8) == 0x02 ||
        (req->cmd >> 8) == 0x03 || (req->cmd >> 8) == 0x04 ||
        (req->cmd >> 8) == 0x05 || (req->cmd >> 8) == 0x06)
		{
			req->msg[ix++] = req->tagType >> 8;
			req->msg[ix++] = req->tagType & 0x00FF;
      SkyeTek_Debug(_T("TTYP:\t%02X%02X\r\n"),req->msg[ix-2],req->msg[ix-1]);
		  if( req->flags & STPV3_TID )
		  {
			  req->msg[ix++] = (unsigned char)req->tidLength;
			  if( req->tidLength > 0 )
        {
          SkyeTek_Debug(_T("TID:\t"));
				  for( iy = 0; iy < req->tidLength && iy < 16; iy++ )
          {
					  req->msg[ix++] = req->tid[iy];
            SkyeTek_Debug(_T("%02X"),req->msg[ix-1]);
          }
          SkyeTek_Debug(_T("\r\n"));
        }
		  }
		}
		if( req->flags & STPV3_AFI )
    {
			req->msg[ix++] = (unsigned char)req->afi;
      SkyeTek_Debug(_T("AFI:\t%02X\r\n"),req->msg[ix-1]);
    }
		if( req->flags & STPV3_SESSION )
    {
			req->msg[ix++] = (unsigned char)req->session;
      SkyeTek_Debug(_T("SESS:\t%02X\r\n"),req->msg[ix-1]);
    }
		state = STPV3_IsAddressOrDataCommand(req->cmd);
		if( state & STPV3_FORMAT_ADDRESS )
		{
			req->msg[ix++] = req->address[0];
			req->msg[ix++] = req->address[1];
      SkyeTek_Debug(_T("ADDR:\t%02X%02X\r\n"),req->msg[ix-2],req->msg[ix-1]);
		}
		if( state & STPV3_FORMAT_BLOCKS )
		{
			req->msg[ix++] = req->numBlocks >> 8;
			req->msg[ix++] = req->numBlocks & 0x00FF;
      SkyeTek_Debug(_T("BLKS:\t%02X%02X\r\n"),req->msg[ix-2],req->msg[ix-1]);
		} 
		if( req->flags & STPV3_DATA )
		{
			req->msg[ix++] = req->dataLength >> 8;
			req->msg[ix++] = req->dataLength & 0x00FF;
      SkyeTek_Debug(_T("DLEN:\t%02X%02X\r\n"),req->msg[ix-2],req->msg[ix-1]);
			if( req->dataLength > 0 )
			{
        SkyeTek_Debug(_T("DATA:\t"));
				for( iy = 0; iy < req->dataLength && iy < 2048; iy++ )
        {
					req->msg[ix++] = req->data[iy];
          SkyeTek_Debug(_T("%02X"),req->msg[ix-1]);
        }
        SkyeTek_Debug(_T("\r\n"));
			}
		}
    /* Set Length */
		len = ix - 1; /* Minus 3 plus 2 byte CRC */
		req->msg[1] = len >> 8;
		req->msg[2] = len & 0x00FF; 
    SkyeTek_Debug(_T("LEN:\t%02X%02X\r\n"),req->msg[1],req->msg[2]);
		/* Calculate CRC */
		crc_check = crc16((unsigned short)0x0000, req->msg+1, (ix-1));
		req->msg[ix++] = crc_check >> 8;
		req->msg[ix++] = crc_check & 0x00FF;
    SkyeTek_Debug(_T("CRC:\t%02X%02X\r\n"),req->msg[ix-2],req->msg[ix-1]);
		req->msgLength = ix;
	}
  
  return SKYETEK_SUCCESS;
}

SKYETEK_API SKYETEK_STATUS STPV3_WriteRequest( 
  LPSKYETEK_DEVICE        lpDevice, 
  LPSTPV3_REQUEST         req,
  unsigned int            timeout
  )
{
	unsigned int written = 0, totalWritten = 0;
	int i = 0;
	char c = 0;
	SKYETEK_STATUS status;
  LPDEVICEIMPL pd;

  if( lpDevice == NULL || req == NULL )
    return SKYETEK_INVALID_PARAMETER;

  pd = (LPDEVICEIMPL)lpDevice->internal;
  if( pd == NULL )
    return SKYETEK_INVALID_PARAMETER;

  if( (status = STPV3_BuildRequest(req)) != SKYETEK_SUCCESS )
		return status;

	STP_DebugMsg(_T("request"), req->msg, req->msgLength, req->isASCII);
	SkyeTek_Debug(_T("code: %s\r\n"), STPV3_LookupCommand(req->cmd));

	while( (written = pd->Write(lpDevice,req->msg+totalWritten,req->msgLength-totalWritten,timeout)) > 0 )
	{
		totalWritten += written;
		if( totalWritten >= req->msgLength )
			break;
	}
	if( written < 0 )
		return SKYETEK_READER_IO_ERROR;
  return SKYETEK_SUCCESS;
}

SKYETEK_STATUS STPV3_ReadResponseImpl(
  LPSKYETEK_DEVICE      lpDevice, 
  LPSTPV3_REQUEST       req, 
  LPSTPV3_RESPONSE      resp,
  unsigned int          timeout
  )
{
  unsigned int bytesRead = 0, totalRead = 0, length = 0, ix = 0, iy = 0;
  unsigned char *ptr = NULL, *tmp = NULL;
  LPDEVICEIMPL pd;

	memset(resp,0,sizeof(STPV3_RESPONSE));

  if( lpDevice == NULL || resp == NULL )
    return SKYETEK_INVALID_PARAMETER;

  pd = (LPDEVICEIMPL)lpDevice->internal;
  if( pd == NULL )
    return SKYETEK_INVALID_PARAMETER;

	SkyeTek_Debug(_T("timeout is: %d ms\r\n"), (timeout+pd->timeout));
  
	if( req->isASCII )
	{
	  /* Read LF  */
	  bytesRead = pd->Read(lpDevice, resp->msg, 1, timeout);
	  if( bytesRead != 1 )
			return SKYETEK_TIMEOUT;

		/* Check response */
		if( resp->msg[0] != STPV3_LF )
		{
			STP_DebugMsg(_T("response"), resp->msg, 1, req->isASCII);
			return SKYETEK_READER_PROTOCOL_ERROR;
		}

		/* Read in rest of message */
		ptr = resp->msg + 1; /* already read first two */
		while((bytesRead = pd->Read(lpDevice, ptr, 1, timeout)))
		{
			totalRead += bytesRead;
			if( *ptr == STPV3_LF && *(ptr-1) == STPV3_CR )
				break;
			ptr += bytesRead;
		}
	
		/* Check for nothing to read */
		if( totalRead == 0 )
			return SKYETEK_TIMEOUT;

		resp->msgLength = totalRead + 1;
		STP_DebugMsg(_T("response"), resp->msg, resp->msgLength, req->isASCII);

		/* Check size */
		if( resp->msgLength <= 1 || resp->msgLength > STPV3_MAX_ASCII_RESPONSE_SIZE )
    {
      resp->msgLength = 0;
			return SKYETEK_READER_PROTOCOL_ERROR;
    }

		/* Copy over */
		ix = 1;
		resp->code = crcGetHexFromASCII(&resp->msg[ix],4);
		ix += 4;

		/* Check for error code */
		if( STPV3_IsErrorResponse(resp->code) )
		{
			if( req->flags & STPV3_CRC && resp->msgLength > 3 ) 
			{
				if(!verifyacrc((resp->msg)+1, resp->msgLength-3, 1))
				{
					return SKYETEK_INVALID_CRC;
				}
			}
			return SKYETEK_SUCCESS;
		}
		
		/* RID */		
		if( req->flags & STPV3_RID )
		{
			resp->rid[0] = crcGetHexFromASCII(&resp->msg[ix],2);
			ix += 2;
			resp->rid[1] = crcGetHexFromASCII(&resp->msg[ix],2);
			ix += 2;
			resp->rid[2] = crcGetHexFromASCII(&resp->msg[ix],2);
			ix += 2;
			resp->rid[3] = crcGetHexFromASCII(&resp->msg[ix],2);
			ix += 2;
		}
		if( resp->code == STPV3_RESP_SELECT_TAG_PASS && !(req->tagType & 0x0000000F) /* auto-detect */ )
		{
			resp->tagType = crcGetHexFromASCII(&resp->msg[ix],4);
			ix += 4;
		}
		length = resp->msgLength-1;
		if( req->flags & STPV3_CRC )
			length -= 4;
		if( length > ix )
		{
			resp->dataLength = crcGetHexFromASCII(&resp->msg[ix],4);
			resp->dataLength *= 2; /* ASCII is twice binary */
			if( resp->dataLength > resp->msgLength-ix || resp->dataLength > 2048 )
      {
        resp->dataLength = 0;
				return SKYETEK_READER_PROTOCOL_ERROR;
      }
			ix += 4;
			if( resp->dataLength > 0 )
			{
				for( iy = 0; iy < resp->dataLength; iy++ )
					resp->data[iy] = resp->msg[ix++];
			}
		}
		
		/* Verify CRC */
		if( req->flags & STPV3_CRC && resp->msgLength > 3 ) 
		{
			if(!verifyacrc((resp->msg)+1, resp->msgLength-3, 1))
			{
				return SKYETEK_INVALID_CRC;
			}
		}
	}

	/* Binary mode */
	else
	{
	  /* Read STX and message length */
	  bytesRead = pd->Read(lpDevice, resp->msg, 3, timeout);
	  if( bytesRead != 3 )
    {
      SkyeTek_Debug(_T("error: could not read 3 bytes: read %d bytes\r\n"), bytesRead);
			return SKYETEK_TIMEOUT;
    }
  
		/* Check response */
		if( resp->msg[0] != STPV3_STX )
		{
			STP_DebugMsg(_T("response"), resp->msg, 3, req->isASCII);
			if( resp->msg[0] == STPV3_NACK )
				return SKYETEK_READER_IN_BOOT_LOAD_MODE;
			else
				return SKYETEK_READER_PROTOCOL_ERROR;
		}

		/* Check message length */
		length = (resp->msg[1] << 8) | resp->msg[2];
		if( length > STPV3_MAX_ASCII_RESPONSE_SIZE )
		{
			STP_DebugMsg(_T("response"), resp->msg, 3, req->isASCII);
			return SKYETEK_READER_PROTOCOL_ERROR;
		}

		/* Read in rest of message */
		totalRead = 0;
		ptr = resp->msg + 3;
		while((bytesRead = pd->Read(lpDevice, ptr, (length - totalRead), timeout)))
		{
			ptr += bytesRead;
			totalRead += bytesRead;
		}
		resp->msgLength = totalRead + 3;

		if( resp->msgLength > STPV3_MAX_ASCII_RESPONSE_SIZE )
		{
      resp->msgLength = 0;
			STP_DebugMsg(_T("response"), resp->msg, 3, req->isASCII);
			return SKYETEK_READER_PROTOCOL_ERROR;
		}

		/* Get code */
		ix = 3;
		resp->code = (resp->msg[ix] << 8) | resp->msg[ix+1];
		ix += 2;

		STP_DebugMsg(_T("response"), resp->msg, resp->msgLength, req->isASCII);
		SkyeTek_Debug(_T("code: %s\r\n"), STPV3_LookupResponse(resp->code));

		/* Check for error code */
		if( STPV3_IsErrorResponse(resp->code) )
		{
			resp->msgLength = ix + 2; /* for CRC */
			if(!verifycrc((resp->msg)+1, length, 1))
				return SKYETEK_INVALID_CRC;
			else
				return SKYETEK_SUCCESS;
		}

		/* Otherwise, process response */
		if( req->flags & STPV3_RID )
		{
			for( iy = 0; iy < 4; iy++ )
				resp->rid[iy] = resp->msg[ix++];
		}
		if( resp->code == STPV3_RESP_SELECT_TAG_PASS && !(req->tagType & 0x0000000F) /* auto-detect */ )
		{
			resp->tagType = (SKYETEK_TAGTYPE)((resp->msg[ix] << 8) | resp->msg[ix+1]);
			ix += 2;
		}
		if( length + 1 > ix )
		{
			resp->dataLength = (resp->msg[ix] << 8) | resp->msg[ix+1];
			if( resp->dataLength > length || resp->dataLength > sizeof(resp->data) )
      {
        resp->dataLength = 0;
				return SKYETEK_READER_PROTOCOL_ERROR;
      }
			ix += 2;
			if( resp->dataLength > 0 )
			{
				for( iy = 0; iy < resp->dataLength; iy++ )
					resp->data[iy] = resp->msg[ix++];
			}
		}
		resp->msgLength = ix + 2; /* for CRC */

		/* Verify CRC */
		if(!verifycrc((resp->msg)+1, length, 1))
			return SKYETEK_INVALID_CRC;
	}

	
	return SKYETEK_SUCCESS;
} 

SKYETEK_API SKYETEK_STATUS STPV3_ReadResponse(
  LPSKYETEK_DEVICE      lpDevice, 
  LPSTPV3_REQUEST       req, 
  LPSTPV3_RESPONSE      resp,
  unsigned int          timeout
  )
{
  SKYETEK_STATUS st;
  unsigned int count = 0;
  
read:
  st = STPV3_ReadResponseImpl(lpDevice,req,resp,timeout);
  count++;
  if( resp->code == 0 || resp->code & 0x00008000 )
  {
    return st;
  }
  else if( req->cmd == STPV3_CMD_SELECT_TAG &&
           resp->code == STPV3_RESP_SELECT_TAG_LOOP_ON )
  {
    return st;
  }
  else if( (resp->code & 0x00007FFF) == (req->cmd & 0x00007FFF) )
  {
    return st;
  }
  else if( req->anyResponse )
  {
    return st;
  }
  else
  {
		SkyeTek_Debug(_T("error: response code 0x%X doesn't match request 0x%X\r\n"), resp->code, req->cmd);
    count++;
    if( count > 10 )
      return st;
    goto read;
  }
}

SKYETEK_API unsigned char STPV3_IsAddressOrDataCommand( unsigned int cmd )
{
  /*
  #define STPV3_FORMAT_NOTHING  0x00
  #define STPV3_FORMAT_ADDRESS  0x01,
  #define STPV3_FORMAT_BLOCKS   0x02,
  #define STPV3_FORMAT_DATA     0x04,
  */
  switch(cmd)
    {
		/* address and blocks - no data */
    case STPV3_CMD_READ_TAG:
    case STPV3_CMD_READ_TAG_CONFIG:
    case STPV3_CMD_READ_SYSTEM_PARAMETER:
    case STPV3_CMD_RETRIEVE_DEFAULT_SYSTEM_PARAMETER:
			return (STPV3_FORMAT_ADDRESS | STPV3_FORMAT_BLOCKS);
			break;
		/* data only - no address */
    case STPV3_CMD_SET_TAG_BIT_RATE:
    case STPV3_CMD_KILL_TAG:
    case STPV3_CMD_AUTHENTICATE_READER:
    case STPV3_CMD_REVIVE_TAG:
    case STPV3_CMD_WRITE_AFI:
    case STPV3_CMD_WRITE_DSFID:
		case STPV3_CMD_CREDIT_VALUE_FILE:
		case STPV3_CMD_DEBIT_VALUE_FILE:
    case STPV3_CMD_SELECT_APPLICATION:
    case STPV3_CMD_SEND_TAG_PASSWORD:
    case STPV3_CMD_INIT_SECURE_MEMORY:
    case STPV3_CMD_SETUP_SECURE_MEMORY:
    case STPV3_CMD_CREATE_APPLICATION:
    case STPV3_CMD_DELETE_APPLICATION:
    case STPV3_CMD_GET_FILE_IDS:
    case STPV3_CMD_SELECT_FILE:
    case STPV3_CMD_CREATE_FILE:
    case STPV3_CMD_GET_FILE_SETTINGS:
    case STPV3_CMD_CHANGE_FILE_SETTINGS:
    case STPV3_CMD_READ_FILE:
    case STPV3_CMD_WRITE_FILE:
    case STPV3_CMD_DELETE_FILE:
    case STPV3_CMD_CLEAR_FILE:
    case STPV3_CMD_LIMITED_CREDIT_VALUE_FILE:
    case STPV3_CMD_GET_VALUE:
    case STPV3_CMD_READ_RECORDS:
    case STPV3_CMD_WRITE_RECORD:
    case STPV3_CMD_GET_KEY_VERSION:
    case STPV3_CMD_CHANGE_KEY:
    case STPV3_CMD_CHANGE_KEY_SETTINGS:
    case STPV3_CMD_GET_KEY_SETTINGS:
    case STPV3_CMD_INTERFACE_SEND:
    case STPV3_CMD_TRANSPORT_SEND:
    case STPV3_CMD_INITIATE_PAYMENT:
    case STPV3_CMD_COMPUTE_PAYMENT:
			return (STPV3_FORMAT_DATA);
			break;
		/* address, blocks and data */
    case STPV3_CMD_WRITE_TAG:
    case STPV3_CMD_WRITE_TAG_CONFIG:
    case STPV3_CMD_WRITE_SYSTEM_PARAMETER:
    case STPV3_CMD_STORE_DEFAULT_SYSTEM_PARAMETER:
			return (STPV3_FORMAT_ADDRESS | STPV3_FORMAT_BLOCKS | STPV3_FORMAT_DATA);
			break;
		/* address but no numblocks or data */
    case STPV3_CMD_ERASE_TAG:
    case STPV3_CMD_GET_LOCK_STATUS:
    case STPV3_CMD_LOAD_KEY:
			return (STPV3_FORMAT_ADDRESS);
			break;
    /* adress with no numblocks but with data */
    case STPV3_CMD_AUTHENTICATE_TAG:
    case STPV3_CMD_STORE_KEY:
      return (STPV3_FORMAT_ADDRESS | STPV3_FORMAT_DATA);
      break;
    default:
      return STPV3_FORMAT_NOTHING;
    }
	return STPV3_FORMAT_NOTHING;
}

SKYETEK_STATUS STPV3_GetStatus(unsigned int code)
{
	int ix;
	for( ix = 0; ix < STPV3_STATUS_LOOKUPS_COUNT; ix++ )
	{
		if( stpv3Statuses[ix].cmd == code )
			return stpv3Statuses[ix].status;
	}
	return SKYETEK_FAILURE;
}


/* CRC calculation */
UINT16 crcBL16(UINT8 *dataP, UINT16 nBytes, UINT16 preset)
{
    UINT16 i, j;
	UINT8 mBits = 8;

  	UINT16 crc_16 = preset;

	for( i=0; i<nBytes; i++ )
	{
	 	crc_16 ^= *dataP++;
		
		for( j=0; j<mBits; j++ )
		{
			if( crc_16 & 0x0001 )
			{
				crc_16 >>= 1;
				crc_16 ^= 0x8408;  // Polynomial (x^16 + x^12 + x^5 + 1)
			}
		    else
			{
				crc_16 >>= 1;
			}
		}
	}

	return( crc_16 );
}

UINT16 verifyBLcrc(UINT8 *resp, UINT16 len)
{
  UINT16 crc_check;
  
	crc_check = crcBL16(resp, len-2, 0x0000);
	if(resp[len-2] == (crc_check >> 8) &&
     resp[len-1] == (crc_check & 0x00FF))
		return 1;
	else
		return 0;
}

/* Function to send commands to the bootloader */
UINT16 
sendBLCommand(
  LPSKYETEK_DEVICE  lpDevice, 
  UINT8           commandCode, 
  UINT8           *commandData, 
  UINT16          numBytes, 
  UINT8           *respString 
  ) 
{
  LPDEVICEIMPL pd;
	union _16Bits crc;
	union _16Bits len;
	UINT8 temp;
  int i = 0, num = 0, bytes = 0, count = 0, totalCount = 0;

  if( lpDevice == NULL )
    return 0;
  pd = (LPDEVICEIMPL)lpDevice->internal;
  if( pd == NULL )
    return 0;

	crc.w = 0x0000;

	len.w = numBytes + 3;	/* Adjust numbytes for the command code and crc bytes */

	/* Calculate the CRC over the nuber of bytes field */
	temp = len.b[1];
	crc.w = crcBL16(&temp, 1, crc.w);
	temp = len.b[0];
	crc.w = crcBL16(&temp, 1, crc.w);

	/* Calculate the CRC over the command Code and command Data */
	crc.w = crcBL16(&commandCode, 1, crc.w);
	crc.w = crcBL16(commandData, numBytes, crc.w);
	
	/* Send the Command to the reader */
sendcommand:
	temp = len.b[1];
	pd->Write(lpDevice, &temp, 1, 100);
	temp = len.b[0];
	pd->Write(lpDevice, &temp, 1, 100);
	pd->Write(lpDevice, &commandCode, 1, 100);
	pd->Write(lpDevice, commandData, numBytes, 500);

	temp = crc.b[1];
	pd->Write(lpDevice, &temp, 1, 100);
	temp = crc.b[0];
	pd->Write(lpDevice, &temp, 1, 100);
	
  pd->Flush(lpDevice);

	/* Wait for the response */
  if( commandCode == SETUP_BOOTLOADER )
    SKYETEK_Sleep(3000);

	/* Now get the response back form the reader */
  count = 0;
readresponse:
	num = pd->Read(lpDevice, respString, 2, 500);
  if( num != 2 )
  {
    count++;
    if( count > 5 )
      return num;
    pd->Flush(lpDevice);
    goto readresponse;
  }

  bytes = (int)respString[1];
  num = 2 + pd->Read(lpDevice,respString+2,bytes,500);

  /* Verify CRC */
  if( !verifyBLcrc(respString,num) )
  {
    totalCount++;
    if( totalCount > 4 )
      return 0;
    goto sendcommand;
  }
  
  /* Check command code */
  if( respString[2] != commandCode )
  {
    totalCount++;
    if( totalCount > 4 )
      return 0;
    goto sendcommand;
  }

  return num;
}



SKYETEK_STATUS 
STPV3_SendCommand(
    LPSKYETEK_READER     lpReader,
    unsigned int         cmd,
    unsigned int         flags,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;

  if(lpReader == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = cmd;
	req.flags = STPV3_CRC;
  req.flags |= flags;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 

}

SKYETEK_STATUS 
STPV3_SendGetCommand(
    LPSKYETEK_READER     lpReader,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;

  if(lpReader == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = cmd;
	req.flags = STPV3_CRC;
  req.flags |= flags;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
  
  *lpData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpData,resp.data,resp.dataLength);
}

SKYETEK_STATUS 
STPV3_SendSetCommand(
    LPSKYETEK_READER     lpReader,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpData == NULL || lpData->data == NULL || lpData->size > 2048 )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = cmd;
	req.flags = STPV3_CRC | STPV3_DATA;
  req.flags |= flags;
	req.dataLength = lpData->size;
  if( req.dataLength > 2048 )
    req.dataLength = 2048;
	for(iy = 0; iy < req.dataLength; iy++)
		req.data[iy] = lpData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;


	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 

}

SKYETEK_STATUS 
STPV3_SendAddrGetCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;

  if(lpReader == NULL || lpAddr == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = cmd;
	req.flags = STPV3_CRC;
  req.flags |= flags;
	req.address[0] = lpAddr->start >> 8;
	req.address[1] = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks; 
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
  
  *lpData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpData,resp.data,resp.dataLength);
}

SKYETEK_STATUS 
STPV3_SendAddrSetCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpAddr == NULL || lpData == NULL ||
    lpData->data == NULL || lpData->size > 2048 )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = cmd;
	req.flags = STPV3_CRC | STPV3_DATA;
  req.flags |= flags;
	req.address[0] = lpAddr->start >> 8;
	req.address[1] = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks;
	req.dataLength = lpData->size;
  if( req.dataLength > 2048 )
    req.dataLength = 2048;
	for(iy = 0; iy < req.dataLength; iy++)
		req.data[iy] = lpData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 
}

SKYETEK_STATUS 
STPV3_SendTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         cmd,
    unsigned int         flags,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = cmd;
	req.flags = STPV3_CRC;
  req.flags |= flags;
	req.session = lpTag->session;
  req.afi = lpTag->afi;
	req.tagType = lpTag->type;
	if( (flags & STPV3_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    if( req.tidLength > 16 )
      req.tidLength = 16;
    for(iy = 0; iy < req.tidLength; iy++)
      req.tid[iy] = lpTag->id->id[iy];
	}
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	/* Send request */
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	/* Read response */
	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
  
  return SKYETEK_SUCCESS;	
}

SKYETEK_STATUS 
STPV3_SendGetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = cmd;
	req.flags = STPV3_CRC;
  req.flags |= flags;
	req.session = lpTag->session;
  req.afi = lpTag->afi;
	req.tagType = lpTag->type;
	if( (flags & STPV3_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    if( req.tidLength > 16 )
      req.tidLength = 16;
    for(iy = 0; iy < req.tidLength; iy++)
      req.tid[iy] = lpTag->id->id[iy];
	}
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
  
  *lpData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpData,resp.data,resp.dataLength);

}

SKYETEK_STATUS 
STPV3_SendSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpData == NULL ||
    lpData->data == NULL || lpData->size > 2048 )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
  req.cmd = cmd;
	req.flags = STPV3_CRC | STPV3_DATA;
  req.flags |= flags;
	req.session = lpTag->session;
  req.afi = lpTag->afi;
	req.tagType = lpTag->type;
	if( (flags & STPV3_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    if( req.tidLength > 16 )
      req.tidLength = 16;
    for(iy = 0; iy < req.tidLength; iy++)
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.dataLength = lpData->size;
  if( req.dataLength > 2048 )
    req.dataLength = 2048;
	for(iy = 0; iy < req.dataLength; iy++)
		req.data[iy] = lpData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

  memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 

}

SKYETEK_STATUS 
STPV3_SendGetSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpSendData == NULL ||
    lpSendData->data == NULL || lpSendData->size > 2048 || lpRecvData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
  req.cmd = cmd;
	req.flags = STPV3_CRC | STPV3_DATA;
  req.flags |= flags;
	req.session = lpTag->session;
  req.afi = lpTag->afi;
	req.tagType = lpTag->type;
	if( (flags & STPV3_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    if( req.tidLength > 16 )
      req.tidLength = 16;
    for(iy = 0; iy < req.tidLength; iy++)
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.dataLength = lpSendData->size;
  if( req.dataLength > 2048 )
    req.dataLength = 2048;
	for(iy = 0; iy < req.dataLength; iy++)
		req.data[iy] = lpSendData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
    
  *lpRecvData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpRecvData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpRecvData,resp.data,resp.dataLength);
}

SKYETEK_STATUS 
STPV3_SendAddrGetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpAddr == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = cmd;
	req.flags = STPV3_CRC;
  req.flags |= flags;
	req.session = lpTag->session;
  req.afi = lpTag->afi;
	req.tagType = lpTag->type;
	if( (flags & STPV3_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    if( req.tidLength > 16 )
      req.tidLength = 16;
    for(iy = 0; iy < req.tidLength; iy++)
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.address[0] = lpAddr->start >> 8;
	req.address[1] = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
  
  *lpData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpData,resp.data,resp.dataLength);

}

SKYETEK_STATUS 
STPV3_SendAddrSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpAddr == NULL || 
      lpData == NULL || lpData->size > 2048 )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = cmd;
	req.flags = STPV3_CRC | STPV3_DATA;
  req.flags |= flags;
	req.session = lpTag->session;
  req.afi = lpTag->afi;
	req.tagType = lpTag->type;
	if( (flags & STPV3_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    if( req.tidLength > 16 )
      req.tidLength = 16;
    for(iy = 0; iy < req.tidLength; iy++)
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.address[0] = lpAddr->start >> 8;
	req.address[1] = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks;
	req.dataLength = lpData->size;
  if( req.dataLength > 2048 )
    req.dataLength = 2048;
	for(iy = 0; iy < req.dataLength; iy++)
		req.data[iy] = lpData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;  

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
  
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
  {
	  // hack around firmware returning same status as inventory done
	  if( resp.code == STPV3_RESP_SELECT_TAG_INVENTORY_DONE )
		  return SKYETEK_FAILURE;
	  else
		return STPV3_GetStatus(resp.code);
  }
  
  return SKYETEK_SUCCESS;	
}

SKYETEK_STATUS 
STPV3_SendAddrGetSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned int         cmd,
    unsigned int         flags,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpTag == NULL || lpSendData == NULL ||
    lpSendData->data == NULL || lpSendData->size > 2048 || lpRecvData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
  req.cmd = cmd;
	req.flags = STPV3_CRC | STPV3_DATA;
  req.flags |= flags;
	req.session = lpTag->session;
  req.afi = lpTag->afi;
	req.tagType = lpTag->type;
	if( (flags & STPV3_TID) && lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    if( req.tidLength > 16 )
      req.tidLength = 16;    for(iy = 0; iy < req.tidLength; iy++)
      req.tid[iy] = lpTag->id->id[iy];
	}
	req.address[0] = lpAddr->start >> 8;
	req.address[1] = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks;
	req.dataLength = lpSendData->size;
  if( req.dataLength > 2048 )
    req.dataLength = 2048;
	for(iy = 0; iy < req.dataLength; iy++)
		req.data[iy] = lpSendData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != cmd)
    return STPV3_GetStatus(resp.code);
    
  *lpRecvData = SkyeTek_AllocateData(resp.dataLength);
  if( *lpRecvData == NULL )
    return SKYETEK_OUT_OF_MEMORY;
  return SkyeTek_CopyBuffer(*lpRecvData,resp.data,resp.dataLength);
}

/* Reader Functions */
SKYETEK_STATUS 
STPV3_StopSelectLoop(
  LPSKYETEK_READER      lpReader,
  unsigned int          timeout
  )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	int ix = 0, iy = 0;

  if(lpReader == NULL)
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = STPV3_CMD_SELECT_TAG;
	req.flags = STPV3_CRC;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

	/* Send request */
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	/* Read response */
readResponse:
	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
  if( status != SKYETEK_SUCCESS )
    return status;
  if( resp.code != STPV3_RESP_SELECT_TAG_LOOP_OFF )
    goto readResponse;
  return status;
}

SKYETEK_STATUS 
STPV3_SelectTags(
  LPSKYETEK_READER             lpReader, 
  SKYETEK_TAGTYPE              tagType, 
  PROTOCOL_TAG_SELECT_CALLBACK callback,
  PROTOCOL_FLAGS               flags,
  void                         *user,
  unsigned int                 timeout
  )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPSKYETEK_DATA lpd;
  LPREADER_IMPL lpri;
	int ix = 0, iy = 0;

  if((lpReader == NULL) || (callback == 0))
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = STPV3_CMD_SELECT_TAG;
	req.flags = STPV3_CRC | ((flags.isInventory == 1) ? STPV3_INV : 0);
  req.flags = req.flags | ((flags.isLoop == 1) ? STPV3_LOOP : 0);
	req.tagType = tagType;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

	/* Send request */
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

readResponse:
	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
  if( status == SKYETEK_TIMEOUT )
  {
    if(!callback(tagType, NULL, user))
    {
      STPV3_StopSelectLoop(lpReader, timeout);
      return SKYETEK_SUCCESS;
    }
    goto readResponse;
  }
	if( status != SKYETEK_SUCCESS )
		return status;

	if( resp.code == STPV3_RESP_SELECT_TAG_LOOP_ON )
		goto readResponse;

	if( resp.code == STPV3_RESP_SELECT_TAG_FAIL || resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF )
    return SKYETEK_SUCCESS;

	if( resp.code == STPV3_RESP_SELECT_TAG_PASS )
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
			STPV3_StopSelectLoop(lpReader,timeout);
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
STPV3_GetTags(
  LPSKYETEK_READER   lpReader, 
  SKYETEK_TAGTYPE    tagType, 
  LPTAGTYPE_ARRAY    **tagTypes, 
  LPSKYETEK_DATA     **lpData, 
  unsigned int       *count,
  unsigned int       timeout
  )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int ix = 0, iy = 0, num = 0, step = 5;

  if(lpReader == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = STPV3_CMD_SELECT_TAG;
	req.flags = STPV3_CRC | STPV3_INV;
	req.tagType = tagType;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

	/* Send request */
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		goto failure;
  
readResponse:
	/* Read response */
	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
    goto failure; /* timeout or error */

	if( resp.code == STPV3_RESP_SELECT_TAG_FAIL || resp.code == STPV3_RESP_SELECT_TAG_INVENTORY_DONE )
		goto success;

	if( resp.code == STPV3_RESP_SELECT_TAG_PASS )
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

  /* Unknown code? */
  SkyeTek_Debug(_T("Unknown response code: 0x%X\r\n"), resp.code);
  goto success;
 
failure:
  if( *tagTypes != NULL )
  {
    free(*tagTypes);
    *tagTypes = NULL;
  }
  if( *lpData != NULL )
  {
    for( ix = 0; ix < num; ix++ )
      SkyeTek_FreeData((*lpData)[ix]);
    free(*lpData);
    *lpData = NULL;
  }
	*count = 0;
	return status;

success:
	*count = num;
  return SKYETEK_SUCCESS;
}

SKYETEK_STATUS 
STPV3_GetTagsWithMask(
  LPSKYETEK_READER   lpReader, 
  SKYETEK_TAGTYPE    tagType, 
  LPSKYETEK_ID	     lpTagIdMask,
  LPTAGTYPE_ARRAY    **tagTypes, 
  LPSKYETEK_DATA     **lpData, 
  unsigned int       *count,
  unsigned int       timeout
  )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
    LPREADER_IMPL lpri;
	unsigned int ix = 0, iy = 0, num = 0, step = 5;

  if(lpReader == NULL || lpData == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;

	/* Build request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = STPV3_CMD_SELECT_TAG;
	req.flags = STPV3_CRC | STPV3_INV;
	req.tagType = tagType;
	if( lpTagIdMask->id != NULL )
	{
		req.tidLength = lpTagIdMask->length;
		for(iy = 0; iy < lpTagIdMask->length && iy < 16; iy++)
			req.tid[iy] = lpTagIdMask->id[iy];
		req.flags |= STPV3_TID;
	}

	lpri = (LPREADER_IMPL)lpReader->internal;
	if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
	{
		lpri->CopyRIDToBuffer(lpReader,req.rid);
		req.flags |= STPV3_RID;
	}

	/* Send request */
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		goto failure;
  
readResponse:
	/* Read response */
	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
    goto success; /* done reading */

	if( resp.code == STPV3_RESP_SELECT_TAG_FAIL || resp.code == STPV3_RESP_SELECT_TAG_INVENTORY_DONE )
		goto success;

	if( resp.code == STPV3_RESP_SELECT_TAG_PASS )
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

  /* Unknown code? */
  SkyeTek_Debug(_T("Unknown response code: 0x%X\r\n"), resp.code);
  goto success;
 
failure:
  if( *tagTypes != NULL )
  {
    free(*tagTypes);
    *tagTypes = NULL;
  }
  if( *lpData != NULL )
  {
    for( ix = 0; ix < num; ix++ )
      SkyeTek_FreeData((*lpData)[ix]);
    free(*lpData);
    *lpData = NULL;
  }
	*count = 0;
	return status;

success:
	*count = num;
  return SKYETEK_SUCCESS;
}


SKYETEK_STATUS 
STPV3_StoreKey(
  LPSKYETEK_READER     lpReader,
  SKYETEK_TAGTYPE      type,
  LPSKYETEK_ADDRESS    lpAddr,
  LPSKYETEK_DATA       lpData,
  unsigned int         timeout
  )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpAddr == NULL || lpData == NULL || 
    lpData->data == NULL || lpData->size > 2048 )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = STPV3_CMD_STORE_KEY;
	req.flags = STPV3_CRC | STPV3_DATA;
	req.tagType = type;
	req.address[0] = lpAddr->start >> 8;
	req.address[1] = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks; /*  0x0001; */
	req.dataLength = lpData->size;
	for(iy = 0; iy < req.dataLength && iy < 2048; iy++)
		req.data[iy] = lpData->data[iy];
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != STPV3_RESP_STORE_KEY_PASS)
    return STPV3_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 
}

SKYETEK_STATUS 
STPV3_LoadKey(
  LPSKYETEK_READER     lpReader, 
  LPSKYETEK_ADDRESS    lpAddr,
  unsigned int         timeout
  )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int iy = 0;

  if(lpReader == NULL || lpAddr == NULL )
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Send request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = STPV3_CMD_LOAD_KEY;
	req.flags = STPV3_CRC;
	req.address[0] = lpAddr->start >> 8;
	req.address[1] = lpAddr->start & 0x00FF;
	req.numBlocks = lpAddr->blocks; /* 0x0001; */
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != STPV3_RESP_LOAD_KEY_PASS)
    return STPV3_GetStatus(resp.code);
    
  return SKYETEK_SUCCESS; 
}


SKYETEK_STATUS 
STPV3_LoadDefaults(
  LPSKYETEK_READER     lpReader,
  unsigned int         timeout
  )
{
  return STPV3_SendCommand(lpReader,STPV3_CMD_LOAD_DEFAULTS,0,timeout);
}

SKYETEK_STATUS 
STPV3_ResetDevice(
  LPSKYETEK_READER     lpReader,
  unsigned int         timeout
  )
{
  return STPV3_SendCommand(lpReader,STPV3_CMD_RESET_DEVICE,0,timeout);
}

SKYETEK_STATUS 
STPV3_Bootload(
  LPSKYETEK_READER     lpReader,
  unsigned int         timeout
  )
{
  return STPV3_SendCommand(lpReader,STPV3_CMD_BOOTLOAD,0,timeout);
}

SKYETEK_STATUS 
STPV3_GetSystemParameter(
  LPSKYETEK_READER             lpReader, 
  LPSKYETEK_ADDRESS            lpAddr,
  LPSKYETEK_DATA               *lpData,
  unsigned int                 timeout
  )
{
  return STPV3_SendAddrGetCommand(lpReader,lpAddr,STPV3_CMD_READ_SYSTEM_PARAMETER,0,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_SetSystemParameter(
  LPSKYETEK_READER             lpReader, 
  LPSKYETEK_ADDRESS            lpAddr,
  LPSKYETEK_DATA               lpData,
  unsigned int                 timeout
  )
{
  return STPV3_SendAddrSetCommand(lpReader,lpAddr,STPV3_CMD_WRITE_SYSTEM_PARAMETER,0,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_GetDefaultSystemParameter(
  LPSKYETEK_READER             lpReader, 
  LPSKYETEK_ADDRESS            lpAddr,
  LPSKYETEK_DATA               *lpData,
  unsigned int                 timeout
  )
{
  return STPV3_SendAddrGetCommand(lpReader,lpAddr,STPV3_CMD_RETRIEVE_DEFAULT_SYSTEM_PARAMETER,0,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_SetDefaultSystemParameter(
  LPSKYETEK_READER             lpReader, 
  LPSKYETEK_ADDRESS            lpAddr,
  LPSKYETEK_DATA               lpData,
  unsigned int                 timeout
  )
{
  return STPV3_SendAddrSetCommand(lpReader,lpAddr,STPV3_CMD_STORE_DEFAULT_SYSTEM_PARAMETER,0,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_AuthenticateReader(
  LPSKYETEK_READER     lpReader, 
  LPSKYETEK_DATA       lpData,
  unsigned int         timeout
  )
{
  return STPV3_SendSetCommand(lpReader,STPV3_CMD_AUTHENTICATE_READER,0,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_EnableDebug(
  LPSKYETEK_READER     lpReader,
  unsigned int         timeout
  )
{
  return STPV3_SendCommand(lpReader,STPV3_CMD_ENABLE_DEBUG,STPV3_RF,timeout);
}

SKYETEK_STATUS 
STPV3_DisableDebug(
  LPSKYETEK_READER     lpReader,
  unsigned int         timeout
  )
{
  return STPV3_SendCommand(lpReader,STPV3_CMD_DISABLE_DEBUG,STPV3_RF,timeout);
}

SKYETEK_STATUS 
STPV3_GetDebugMessages(
  LPSKYETEK_READER     lpReader,
  LPSKYETEK_DATA       *lpData,
  unsigned int         timeout
  )
{
  return STPV3_SendGetCommand(lpReader,STPV3_CMD_GET_DEBUG_MESSAGES,STPV3_RF,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_UploadFirmware(
    LPSKYETEK_READER                      lpReader,
    TCHAR                                  *file, 
    unsigned char                         defaultsOnly,
    SKYETEK_FIRMWARE_UPLOAD_CALLBACK      callback, 
    void                                  *user
  )
{
	/* Variable Declarations and Initializations */
	UINT8 blInitString[9];
	FILE* shfFile;
	UINT8 tempBuf[16];
	UINT8 encryptedDataBuf[512];
	UINT8 responseBuf[512];
	union _32Bits dataLen;
	UINT8 commandCode;
	UINT32 i, retryCount;
	union _16Bits structLen;
	union _16Bits structVer;
	union _16Bits blockLen;
	union _32Bits blVer;
	UINT8 encryptionScheme;
	UINT16 byteCount;
	UINT8 sysParamStrLen;
	UINT8 sysParamBuf[100];
  UINT8 pr = 0;
  long minFileSize = 8;
  size_t nr = 0;
  unsigned int ver = 0;
  long st_size = 0;

#ifndef WINCE
#ifdef WIN32
  struct _stat stb;
#else
  struct stat stb;
#endif
#endif

  /* Check inputs */
  if( lpReader == NULL || callback == NULL || file == NULL )
    return SKYETEK_INVALID_PARAMETER;

  /* Initialize the byteCount variable */
	byteCount = 0;

  #ifndef WINCE
  /* Verify file length */
  #ifdef WIN32
  if(_tstat( file, &stb ))
    return SKYETEK_FIRMWARE_BAD_FILE;
  #else
  if(stat( file, &stb ))
    return SKYETEK_FIRMWARE_BAD_FILE;
  #endif

  st_size = stb.st_size;
  #else
  fseek(file, 0, SEEK_END);
  st_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  #endif

  if( st_size < minFileSize )
    return SKYETEK_FIRMWARE_BAD_FILE;

	/* Try and open the file */
	shfFile = _tfopen( file, _T("rb"));
	if(shfFile == NULL)
		return SKYETEK_FIRMWARE_BAD_FILE;
	
	/* Now start reading from the file and copy everything into different fields */

  /* Some progress bar movement */
  if( !callback(5,ver,user) )
  {
    fclose(shfFile);
		return SKYETEK_FIRMWARE_CANCELED;
  }

	/* Read the length of the structure from the shf file */
	nr = fread( tempBuf, 1, 2, shfFile );
  if( nr < 1)
	{
    fclose(shfFile);
		return SKYETEK_FIRMWARE_BAD_FILE;
	}
	structLen.b[1] = tempBuf[0];
	structLen.b[0] = tempBuf[1];

  /* Some progress bar movement */
  if( !callback(6,ver,user) )
  {
    fclose(shfFile);
		return SKYETEK_FIRMWARE_CANCELED;
  }

	/* Copy the Structure Version */
	nr = fread( tempBuf, 1, 2, shfFile );
  if( nr < 1)
	{
    fclose(shfFile);
		return SKYETEK_FIRMWARE_BAD_FILE;
	}
	structVer.b[1] = tempBuf[0];
	structVer.b[0] = tempBuf[1];
	
	byteCount += 2;

	/* Copy the BL Version */
	nr = fread( tempBuf, 1, 4, shfFile );
	if( nr < 1)
	{
    fclose(shfFile);
		return SKYETEK_FIRMWARE_BAD_FILE;
	}
  blVer.b[3] = tempBuf[0];
	blVer.b[2] = tempBuf[1];
	blVer.b[1] = tempBuf[2];
	blVer.b[0] = tempBuf[3];
  ver = blVer.l;

  /* update GUI with version */
  if( !callback(6,ver,user) )
  {
    fclose(shfFile);
		return SKYETEK_FIRMWARE_CANCELED;
  }

	byteCount += 4;

	/* Copy the Encryption Scheme */
	nr = fread( tempBuf, 1, 1, shfFile );
	if( nr < 1)
	{
    fclose(shfFile);
		return SKYETEK_FIRMWARE_BAD_FILE;
	}
  encryptionScheme = tempBuf[0];
	
	byteCount += 1;

  /* Some progress bar movement */
  if( !callback(7,ver,user) )
  {
    fclose(shfFile);
		return SKYETEK_FIRMWARE_CANCELED;
  }

	/* Copy the Bootloader initialization String */
	nr = fread( &blInitString[0], 1, 1, shfFile );
  if( nr < 1)
	{
    fclose(shfFile);
		return SKYETEK_FIRMWARE_BAD_FILE;
	}

  /* Check blInitString[0] */
  /* blInitString can be 0 for non CBC ciphers */
  if( blInitString[0] > 8 )
  {
    fclose(shfFile);
		return SKYETEK_FIRMWARE_BAD_FILE;
  }

  if(blInitString[0])
  {
	/* Make sure we have enough */
	minFileSize += blInitString[0];
	if( st_size < minFileSize )
	{
  		fclose(shfFile);
		return SKYETEK_FIRMWARE_BAD_FILE;
	}  
	nr = fread( &blInitString[1], 1, blInitString[0], shfFile );
	if( nr < 1)
	{
		fclose(shfFile);
		return SKYETEK_FIRMWARE_BAD_FILE;
	}
	
	
	byteCount += blInitString[0];
  }
  byteCount += 1;
	/* Read the Bootloader Version and Make sure that this shf file can be supported by the bootloader */
	commandCode = QUERY_BOOTLDR_VER;
	if( sendBLCommand(lpReader->lpDevice, commandCode, tempBuf, 0, responseBuf) == 0 )
  {
    fclose(shfFile);
		return SKYETEK_FIRMWARE_READER_ERROR;
  }

  /* Some progress bar movement */
  if( !callback(8,ver,user) )
  {
    fclose(shfFile);
		return SKYETEK_FIRMWARE_CANCELED;
  }
  
	/* Seek to the encrypted data block portion of the shf file */
	fseek( shfFile, structLen.w + 2, SEEK_SET );

	/* Copy the Encrypted Block Length from the shf file */
	/* This is a 4 byte number */
	nr = fread(tempBuf, 1, 4, shfFile);
	if( nr < 1)
	{
    fclose(shfFile);
		return SKYETEK_FIRMWARE_BAD_FILE;
	}
  dataLen.b[3] = tempBuf[0];
	dataLen.b[2] = tempBuf[1];
	dataLen.b[1] = tempBuf[2];
	dataLen.b[0] = tempBuf[3];

  /* Make sure we have enough */
  minFileSize += dataLen.l;
  if( st_size < minFileSize )
  {
  	fclose(shfFile);
    return SKYETEK_FIRMWARE_BAD_FILE;
  }

	/* Set the Encryption Scheme */
	commandCode = SELECT_ENCRYPTION_SCHEME;
	tempBuf[0] = encryptionScheme;	/* Choose the encryption scheme */
	if( sendBLCommand(lpReader->lpDevice,commandCode, tempBuf, 1, responseBuf) == 0 )
  {
    fclose(shfFile);
		return SKYETEK_FIRMWARE_READER_ERROR;
  }

  /* Some progress bar movement */
  if( !callback(9,ver,user) )
  {
    fclose(shfFile);
		return SKYETEK_FIRMWARE_CANCELED;
  }

  /* Only setup bootloader if we have data to write */
  if( (defaultsOnly == 0) && (dataLen.l > 0) )
  {
	  /* Send the Bootloader Initialization Sequence to the Reader */
	  commandCode = SETUP_BOOTLOADER;
	  if( sendBLCommand(lpReader->lpDevice, commandCode, blInitString, 9, responseBuf) == 0 )
    {
      fclose(shfFile);
		  return SKYETEK_FIRMWARE_READER_ERROR;
    }

    /* Some progress bar movement */
    if( !callback(10,ver,user) )
    {
      fclose(shfFile);
		  return SKYETEK_FIRMWARE_CANCELED;
    }

	  /* Start sending the encrypted blocks of data */	
	  i = 0;
	  retryCount = 0;
	  commandCode = WRITE_DATA;
	  while(i < dataLen.l)
	  {
		  /* Read out the size of the data block first... */
		  nr = fread(encryptedDataBuf, 1, 2,shfFile);

		  if(nr < 1)
		  {
			fclose(shfFile);
			return SKYETEK_FIRMWARE_BAD_FILE;
		  }

		  blockLen.b[1] = encryptedDataBuf[0];
		  blockLen.b[0] = encryptedDataBuf[1];
		  
		  /* Read from the shf file -- used to be 270 */
		  nr = fread((encryptedDataBuf + 2), 1, blockLen.w, shfFile);
		  
		  if( nr < 1 )
	      {
			fclose(shfFile);
		    return SKYETEK_FIRMWARE_BAD_FILE;
	      }

		  /* Send the upgrade command + data to the reader */
		  /* Account for blockLen */
		  nr +=2; 
		  commandCode = WRITE_DATA;
		  if( sendBLCommand(lpReader->lpDevice, commandCode, encryptedDataBuf, nr, responseBuf) == 0 )
      {
        fclose(shfFile);
		    return SKYETEK_FIRMWARE_READER_ERROR;
      }

		  /* Update the counter */
		  i += nr;
	  
      /* Some progress bar movement */
      pr = (10 + (i*84/dataLen.l));
      if( !callback(pr,ver,user) )
      {
        fclose(shfFile);
		    return SKYETEK_FIRMWARE_CANCELED;
      }
	  }
  } /* Done writing bootloader code */
	
  retryCount = 0;

	/* Check if there are any default system parameters to be programmed */
	if(structLen.w > byteCount )
	{
		/* Point to the default system parameter section in the structure in the shf file */
		fseek( shfFile, byteCount + 2, SEEK_SET );

		/* If there are then first read them out of the shf file and program all the defaults (one at a time) */
		while( structLen.w > byteCount )
		{
			nr = fread( &sysParamStrLen, 1, 1, shfFile );
      if( nr < 1)
	    {
        fclose(shfFile);
		    return SKYETEK_FIRMWARE_BAD_FILE;
	    }
			byteCount += 1;

      if(sysParamStrLen < 1 || sysParamStrLen > 100)
      {
        fclose(shfFile);
		    return SKYETEK_FIRMWARE_BAD_FILE;
      }

			nr = fread( sysParamBuf, 1, sysParamStrLen, shfFile );
      if( nr < 1)
	    {
        fclose(shfFile);
		    return SKYETEK_FIRMWARE_BAD_FILE;
	    }
			byteCount += sysParamStrLen;
						
			/* Send the Command to the reader */
			commandCode = PROGRAM_DEFAULTS;
			if( sendBLCommand(lpReader->lpDevice, commandCode, sysParamBuf, sysParamStrLen, responseBuf) == 0 )
      {
        fclose(shfFile);
		    return SKYETEK_FIRMWARE_READER_ERROR;
      }
			
      /* Some progress bar movement */
      pr = (95 + (byteCount*5/structLen.w));
      if( !callback(pr,ver,user) )
      {
        fclose(shfFile);
		    return SKYETEK_FIRMWARE_CANCELED;
      }
		}
	}

  /* Now send the Firmware Complete Reset command to the reader */
	commandCode = UPDATE_COMPLETE_RESET;
	sendBLCommand(lpReader->lpDevice, commandCode, tempBuf, 0, responseBuf);

  /* Give it time to reset */
  SKYETEK_Sleep(1000);
  callback(100,ver,user);

	/* Close the shf file */
	fclose(shfFile);
	
	/* Return from the function */
	return SKYETEK_SUCCESS;
}

SKYETEK_STATUS 
STPV3_EnterPaymentScanMode(LPSKYETEK_READER lpReader,
                           unsigned int timeout)
{
  return STPV3_SendCommand(lpReader,STPV3_CMD_ENTER_PAYMENT_SCAN_MODE,0,timeout);
}

SKYETEK_STATUS 
STPV3_ScanPayments(
  LPSKYETEK_READER              lpReader,
  SKYETEK_PAYMENT_SCAN_CALLBACK callback,
  void                          *user,
  unsigned int                  timeout
  )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
  char line[256];
  char *ptr;
  int r;
  int count = 0;

  if(lpReader == NULL || lpReader->lpDevice == NULL || 
     lpReader->internal == NULL || callback == NULL )
    return SKYETEK_INVALID_PARAMETER;
  
	/* Build request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = STPV3_CMD_ENTER_PAYMENT_SCAN_MODE;
	req.flags = STPV3_CRC;
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

  /* Send request */
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

  /* Get response */
	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

  /* Loop reading responses */
  while( 1 )
  {
    memset(line,0,256);
    ptr = line;
    count = 0;
    while( 1 )
    {
      r = SkyeTek_ReadDevice(lpReader->lpDevice,(unsigned char *)ptr,1,100);
      if( r == -1 )
        return SKYETEK_READER_IO_ERROR;
      if( r == 0 )
      {
        if( !callback(NULL,user) )
          goto forceExit;
        continue;
      }
      if( *ptr == '\n' )
      {
        ptr--;
        break;
      }
      count++;
      if( count == 255 )
        break;
      ptr++;
    }
    *ptr = '\0';
    
    if( strcmp(line,"--") == 0 )
    {
      return SKYETEK_SUCCESS;
    }
    else
    {
      if( !callback(line,user) )
        goto forceExit;
    }
  }
  return SKYETEK_FAILURE;

forceExit:
  // signal cancel with generic command
  STPV3_SendCommand(lpReader,STPV3_CMD_SELECT_TAG,0,timeout);
  return SKYETEK_SUCCESS;
}

/* Tag functions */
SKYETEK_STATUS 
STPV3_SelectTag(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
	STPV3_REQUEST req;
	STPV3_RESPONSE resp;
	SKYETEK_STATUS status;
  LPREADER_IMPL lpri;
	unsigned int ix = 0, iy = 0;
  TCHAR *str;

  if(lpReader == NULL || lpTag == NULL)
    return SKYETEK_INVALID_PARAMETER;
  if( lpReader->lpDevice == NULL || lpReader->internal == NULL)
    return SKYETEK_INVALID_PARAMETER;
  
	/* Build request */
	memset(&req,0,sizeof(STPV3_REQUEST));
	req.cmd = STPV3_CMD_SELECT_TAG;
	req.flags = STPV3_CRC;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    req.flags |= STPV3_TID;
  req.flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	req.flags |= (lpTag->afi > 0 ? STPV3_AFI : 0);
	req.flags |= (lpTag->session > 0 ? STPV3_SESSION : 0);
	req.session = lpTag->session;
	req.afi = lpTag->afi;
	req.tagType = lpTag->type;
	if( lpTag->id != NULL && lpTag->id->id != NULL )
	{
		req.tidLength = lpTag->id->length;
    for(iy = 0; iy < lpTag->id->length && iy < 16; iy++)
      req.tid[iy] = lpTag->id->id[iy];
	}
  lpri = (LPREADER_IMPL)lpReader->internal;
  if( lpReader->sendRID || !lpri->DoesRIDMatch(lpReader,genericID) )
  {
    lpri->CopyRIDToBuffer(lpReader,req.rid);
    req.flags |= STPV3_RID;
  }

writeCommand:
	/* Send request */
	status = STPV3_WriteRequest(lpReader->lpDevice, &req, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;

	/* Read response */
	memset(&resp,0,sizeof(STPV3_RESPONSE));
	status = STPV3_ReadResponse(lpReader->lpDevice, &req, &resp, timeout);
	if( status != SKYETEK_SUCCESS )
		return status;
	
	/* Process response */
  if(resp.code == STPV3_RESP_SELECT_TAG_LOOP_OFF)
    goto writeCommand;
  else if(resp.code != STPV3_RESP_SELECT_TAG_PASS)
    return STPV3_GetStatus(resp.code);
  
  if(resp.tagType != 0)
  {
    lpTag->type = (SKYETEK_TAGTYPE)resp.tagType;
    InitializeTagType(lpTag);
  }
  
  /* If TID Flag is not set, then populate the tag ID into the tag buffer */
  if(req.flags & STPV3_TID)
	{
		lpTag->session = resp.data[0];
	}
	else
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
STPV3_ReadTagData(    
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    PROTOCOL_FLAGS       flags,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  unsigned int f = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    f |= STPV3_TID;
  if( flags.isEncrypt )
    f |= STPV3_ENCRYPTION;
  if( flags.isHMAC )
    f |= STPV3_HMAC;
  f |= (lpTag->rf > 0 ? STPV3_RF : 0);
  f |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendAddrGetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_READ_TAG,f,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_WriteTagData(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    PROTOCOL_FLAGS       flags,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int f = 0;
  if( flags.isEncrypt )
    f |= STPV3_ENCRYPTION;
  if( flags.isHMAC )
    f |= STPV3_HMAC;
  if( flags.isLock )
    f |= STPV3_LOCK;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    f |= STPV3_TID;
  f |= (lpTag->rf > 0 ? STPV3_RF : 0);
  f |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendAddrSetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_WRITE_TAG,f,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_ReadTagConfig(    
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    PROTOCOL_FLAGS       flags,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  unsigned int f = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    f |= STPV3_TID;
  f |= (lpTag->rf > 0 ? STPV3_RF : 0);
  f |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendAddrGetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_READ_TAG_CONFIG,f,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_WriteTagConfig(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    PROTOCOL_FLAGS       flags,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int f = 0;
  if( flags.isLock )
    f |= STPV3_LOCK;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    f |= STPV3_TID;
  f |= (lpTag->rf > 0 ? STPV3_RF : 0);
  f |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendAddrSetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_WRITE_TAG_CONFIG,f,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_ActivateTagType(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV3_DeactivateTagType(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV3_SetTagBitRate(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  return SKYETEK_NOT_SUPPORTED;
}

SKYETEK_STATUS 
STPV3_GetTagInfo(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendGetTagCommand(lpReader,lpTag,STPV3_CMD_GET_TAG_INFO,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_GetLockStatus(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
  return STPV3_SendAddrGetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_GET_LOCK_STATUS,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_KillTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_KILL_TAG,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_ReviveTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendTagCommand(lpReader,lpTag,STPV3_CMD_REVIVE_TAG,flags,timeout);
}

SKYETEK_STATUS 
STPV3_EraseTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag, 
    unsigned int         timeout 
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendTagCommand(lpReader,lpTag,STPV3_CMD_ERASE_TAG,flags,timeout);
}

SKYETEK_STATUS 
STPV3_FormatTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendTagCommand(lpReader,lpTag,STPV3_CMD_FORMAT_TAG,flags,timeout);
}

SKYETEK_STATUS 
STPV3_DeselectTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendTagCommand(lpReader,lpTag,STPV3_CMD_DESELECT_TAG,flags,timeout);
}

SKYETEK_STATUS 
STPV3_AuthenticateTag(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendAddrSetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_AUTHENTICATE_TAG,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_SendTagPassword(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_SEND_TAG_PASSWORD,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_GetApplicationIDs(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetTagCommand(lpReader,lpTag,STPV3_CMD_GET_APPLICATION_IDS,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_SelectApplication(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION; 
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_SELECT_APPLICATION,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_CreateApplication(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_CREATE_APPLICATION,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_DeleteApplication(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_DELETE_APPLICATION,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_GetFileIDs(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetTagCommand(lpReader,lpTag,STPV3_CMD_GET_FILE_IDS,flags,lpRecvData,timeout);
}

SKYETEK_STATUS 
STPV3_SelectFile(
    LPSKYETEK_READER    lpReader,
    LPSKYETEK_TAG       lpTag,
    LPSKYETEK_DATA      lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_SELECT_FILE,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_CreateFile(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_CREATE_FILE,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_GetFileSettings(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetSetTagCommand(lpReader,lpTag,STPV3_CMD_GET_FILE_SETTINGS,flags,lpSendData,lpRecvData,timeout);
}

SKYETEK_STATUS 
STPV3_ChangeFileSettings(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_CHANGE_FILE_SETTINGS,flags,lpSendData,timeout);
}

SKYETEK_STATUS 
STPV3_ReadFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendAddrGetSetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_READ_FILE,flags,lpSendData,lpRecvData,timeout);
}

SKYETEK_STATUS 
STPV3_WriteFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendAddrSetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_WRITE_FILE,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_DeleteFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_DELETE_FILE,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_ClearFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_CLEAR_FILE,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_CreditValueFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_CREDIT_VALUE_FILE,flags,lpData,timeout);
}


SKYETEK_STATUS 
STPV3_DebitValueFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_DEBIT_VALUE_FILE,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_LimitedCreditValueFile(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_LIMITED_CREDIT_VALUE_FILE,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_GetValue(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetSetTagCommand(lpReader,lpTag,STPV3_CMD_GET_VALUE,flags,lpSendData,lpRecvData,timeout);
}

SKYETEK_STATUS 
STPV3_ReadRecords(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendAddrGetSetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_READ_RECORDS,flags,lpSendData,lpRecvData,timeout);
}
    	
SKYETEK_STATUS 
STPV3_WriteRecord(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendAddrSetTagCommand(lpReader,lpTag,lpAddr,STPV3_CMD_WRITE_RECORD,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_CommitTransaction(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendTagCommand(lpReader,lpTag,STPV3_CMD_COMMIT_TRANSACTION,flags,timeout);
}

SKYETEK_STATUS 
STPV3_AbortTransaction(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendTagCommand(lpReader,lpTag,STPV3_CMD_ABORT_TRANSACTION,flags,timeout);
}

SKYETEK_STATUS 
STPV3_EnableEAS(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
  return STPV3_SendTagCommand(lpReader,lpTag,STPV3_CMD_ENABLE_EAS,flags,timeout);
}

SKYETEK_STATUS 
STPV3_DisableEAS(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
  return STPV3_SendTagCommand(lpReader,lpTag,STPV3_CMD_DISABLE_EAS,flags,timeout);
}

SKYETEK_STATUS 
STPV3_ScanEAS(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag,
    unsigned int         timeout
    )
{
  return STPV3_SendTagCommand(lpReader,lpTag,STPV3_CMD_SCAN_EAS,0,timeout);
}

SKYETEK_STATUS 
STPV3_ReadAFI(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
  return STPV3_SendGetTagCommand(lpReader,lpTag,STPV3_CMD_READ_AFI,flags,lpData,timeout);
}
    	
SKYETEK_STATUS 
STPV3_WriteAFI(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_WRITE_AFI,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_ReadDSFID(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       *lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
  return STPV3_SendGetTagCommand(lpReader,lpTag,STPV3_CMD_READ_DSFID,flags,lpData,timeout);
}
    	
SKYETEK_STATUS 
STPV3_WriteDSFID(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_TAG        lpTag, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_WRITE_DSFID,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_GetKeyVersion(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetSetTagCommand(lpReader,lpTag,STPV3_CMD_GET_KEY_VERSION,flags,lpSendData,lpRecvData,timeout);
}

SKYETEK_STATUS 
STPV3_ChangeKey(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_CHANGE_KEY,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_GetKeySettings(
    LPSKYETEK_READER        lpReader,
    LPSKYETEK_TAG           lpTag,
    LPSKYETEK_DATA          *lpRecvData,
    unsigned int            timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetTagCommand(lpReader,lpTag,STPV3_CMD_GET_KEY_SETTINGS,flags,lpRecvData,timeout);
}


SKYETEK_STATUS 
STPV3_ChangeKeySettings(
    LPSKYETEK_READER             lpReader,
    LPSKYETEK_TAG                lpTag,
    LPSKYETEK_DATA               lpData,
    unsigned int                 timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_CHANGE_KEY_SETTINGS,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_InitializeSecureMemoryTag(
    LPSKYETEK_READER          lpReader,
    LPSKYETEK_TAG             lpTag,
    LPSKYETEK_DATA            lpData,
    unsigned int              timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_INIT_SECURE_MEMORY,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_SetupSecureMemoryTag(
    LPSKYETEK_READER          lpReader,
    LPSKYETEK_TAG             lpTag,
    LPSKYETEK_DATA            lpData,
    unsigned int              timeout
    )
{
  unsigned int flags = 0;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= (lpTag->session > 0 ? STPV3_SESSION : 0);
  return STPV3_SendSetTagCommand(lpReader,lpTag,STPV3_CMD_SETUP_SECURE_MEMORY,flags,lpData,timeout);
}

SKYETEK_STATUS 
STPV3_InterfaceSend(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetSetTagCommand(lpReader,lpTag,STPV3_CMD_INTERFACE_SEND,flags,lpSendData,lpRecvData,timeout);
}

SKYETEK_STATUS 
STPV3_TransportSend(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetSetTagCommand(lpReader,lpTag,STPV3_CMD_TRANSPORT_SEND,flags,lpSendData,lpRecvData,timeout);
}

SKYETEK_STATUS 
STPV3_InitiatePayment(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetTagCommand(lpReader,lpTag,STPV3_CMD_INITIATE_PAYMENT,flags,lpRecvData,timeout);
}

SKYETEK_STATUS 
STPV3_ComputePayment(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int         timeout
    )
{
  unsigned int flags = 0;
  if( lpTag->id != NULL && lpTag->id->id != NULL && lpTag->id->length > 0 )
    flags |= STPV3_TID;
  flags |= (lpTag->rf > 0 ? STPV3_RF : 0);
	flags |= STPV3_SESSION;
  return STPV3_SendGetSetTagCommand(lpReader,lpTag,STPV3_CMD_COMPUTE_PAYMENT,flags,lpSendData,lpRecvData,timeout);
}

unsigned char STPV3_IsErrorResponse(unsigned int code)
{
	return( ((code & 0x0000F000) == 0x00008000) || ((code & 0x0000F000) == 0x00009000));
}

TCHAR *STPV3_LookupCommand( unsigned int cmd )
{
	int ix;
	for( ix = 0; ix < STPV3_CMD_LOOKUPS_COUNT; ix++ )
	{
		if( stpv3Commands[ix].cmd == cmd )
			return stpv3Commands[ix].name;
	}
	return _T("Unknown");
}

unsigned int STPV3_LookupCommandCode( TCHAR *cmdName )
{
	int ix;
  if( cmdName == NULL )
    return 0;
	for( ix = 0; ix < STPV3_CMD_LOOKUPS_COUNT; ix++ )
	{
		if( _tcscmp(stpv3Commands[ix].name, cmdName) == 0 )
			return stpv3Commands[ix].cmd;
	}
	return 0;
}

int STPV3_GetCommandCount()
{
	return STPV3_CMD_LOOKUPS_COUNT;
}

TCHAR *STPV3_GetCommandNameAt( int ix )
{
	if( ix >= 0 && ix < STPV3_CMD_LOOKUPS_COUNT )
		return stpv3Commands[ix].name;
	return _T("Unknown");
}

TCHAR *STPV3_LookupResponse( unsigned int resp )
{
	int ix;
	for( ix = 0; ix < STPV3_RESP_LOOKUPS_COUNT; ix++ )
	{
		if( stpv3Responses[ix].cmd == resp )
			return stpv3Responses[ix].name;
	}
	return _T("Unknown");
}

unsigned int STPV3_LookupResponseCode( TCHAR *respName )
{
	int ix;
  if( respName == NULL )
    return 0;

	for( ix = 0; ix < STPV3_RESP_LOOKUPS_COUNT; ix++ )
	{
		if( _tcscmp(stpv3Responses[ix].name, respName) == 0 )
			return stpv3Responses[ix].cmd;
	}
	return 0;
}

int STPV3_GetResponsesCount()
{
	return STPV3_RESP_LOOKUPS_COUNT;
}

TCHAR *STPV3_GetResponseNameAt( int ix )
{
	if( ix >= 0 && ix < STPV3_RESP_LOOKUPS_COUNT )
		return stpv3Responses[ix].name;
	return _T("Unknown");
}


PROTOCOLIMPL STPV3Impl = {
  3,
  STPV3_SelectTags,
  STPV3_GetTags,
  STPV3_GetTagsWithMask,
  STPV3_StoreKey,
  STPV3_LoadKey,
  STPV3_LoadDefaults,
  STPV3_ResetDevice,
  STPV3_Bootload,
  STPV3_GetSystemParameter,
  STPV3_SetSystemParameter,
  STPV3_GetDefaultSystemParameter,
  STPV3_SetDefaultSystemParameter,
  STPV3_AuthenticateReader,
  STPV3_EnableDebug,
  STPV3_DisableDebug,
  STPV3_UploadFirmware,
  STPV3_EnterPaymentScanMode,
  STPV3_ScanPayments,
  STPV3_SelectTag,
  STPV3_ReadTagData,    
  STPV3_WriteTagData,
  STPV3_ReadTagConfig,    
  STPV3_WriteTagConfig,
  STPV3_ActivateTagType,
  STPV3_DeactivateTagType,
  STPV3_SetTagBitRate,
  STPV3_GetTagInfo,
  STPV3_GetLockStatus,
  STPV3_KillTag,
  STPV3_ReviveTag,
  STPV3_EraseTag,
  STPV3_FormatTag,
  STPV3_DeselectTag,
  STPV3_AuthenticateTag,
  STPV3_SendTagPassword,
  STPV3_GetApplicationIDs,
  STPV3_SelectApplication,
  STPV3_CreateApplication,
  STPV3_DeleteApplication,
  STPV3_GetFileIDs,
  STPV3_SelectFile,
  STPV3_CreateFile,
  STPV3_GetFileSettings,
  STPV3_ChangeFileSettings,
  STPV3_ReadFile,
  STPV3_WriteFile,
  STPV3_DeleteFile,
  STPV3_ClearFile,
  STPV3_CreditValueFile,
  STPV3_DebitValueFile,
  STPV3_LimitedCreditValueFile,
  STPV3_GetValue,
  STPV3_ReadRecords,
  STPV3_WriteRecord,
  STPV3_CommitTransaction,
  STPV3_AbortTransaction,
  STPV3_EnableEAS,
  STPV3_DisableEAS,
  STPV3_ScanEAS,
  STPV3_ReadAFI,
  STPV3_WriteAFI,
  STPV3_ReadDSFID,
  STPV3_WriteDSFID,
  STPV3_GetKeyVersion,
  STPV3_ChangeKey,
  STPV3_GetKeySettings,
  STPV3_ChangeKeySettings,
  STPV3_InitializeSecureMemoryTag,
  STPV3_SetupSecureMemoryTag,
  STPV3_InterfaceSend,
  STPV3_TransportSend,
  STPV3_InitiatePayment,
  STPV3_ComputePayment,
  STPV3_GetDebugMessages
};
