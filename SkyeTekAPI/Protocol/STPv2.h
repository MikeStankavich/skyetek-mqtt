/**
 * STPv2.h
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Header file for SkyeTek Protocol version 2
 */
#ifndef SKYETEK_STPV2_H
#define SKYETEK_STPV2_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../SkyeTekAPI.h"

/**
 * SkytekProtocol command flags
 */
typedef enum {
  STPV2_LOOP = 0x01,
  STPV2_INV = 0x02,
  STPV2_LOCK = 0x04,
  STPV2_RF = 0x08,
  STPV2_AFI = 0x10,
  STPV2_CRC = 0x20,
  STPV2_TID = 0x40,
  STPV2_RID = 0x80
} STPV2_FLAG;

  /*Framing*/
#define STPV2_CR 0x0D
#define STPV2_LF 0x0A
#define STPV2_STX 0x02
#define STPV2_ACK 0x06
#define STPV2_NACK 0x15


#define STPV2_TIMEOUT 2000

/* Commands */
#define STPV2_CMD_SELECT_TAG  	0x14
#define STPV2_CMD_READ_MEMORY		0x21
#define STPV2_CMD_READ_SYSTEM 	0x22
#define STPV2_CMD_READ_TAG    	0x24
#define STPV2_CMD_WRITE_MEMORY	0x41
#define STPV2_CMD_WRITE_SYSTEM 	0x42
#define STPV2_CMD_WRITE_TAG   	0x44

/*Response*/
#define STPV2_RESP_SELECT_TAG_PASS 		  0x14
#define STPV2_RESP_SELECT_TAG_LOOP_ON 	0x1C
#define STPV2_RESP_SELECT_TAG_FAIL		  0x94
#define STPV2_RESP_SELECT_TAG_LOOP_OFF 	0x9C
#define STPV2_RESP_READ_MEMORY_PASS		  0x21
#define STPV2_RESP_READ_SYSTEM_PASS		  0x22
#define STPV2_RESP_READ_TAG_PASS 		    0x24
#define STPV2_RESP_READ_MEMORY_FAIL		  0xA1
#define STPV2_RESP_READ_SYSTEM_FAIL		  0xA2
#define STPV2_RESP_READ_TAG_FAIL 		    0xA4
#define STPV2_RESP_EVENT_REPORT					0x32
#define STPV2_RESP_EVENT_FAIL						0xC2
#define STPV2_RESP_WRITE_MEMORY_PASS		0x41
#define STPV2_RESP_WRITE_SYSTEM_PASS		0x42
#define STPV2_RESP_WRITE_TAG_PASS 		  0x44
#define STPV2_RESP_WRITE_MEMORY_FAIL		0xC1
#define STPV2_RESP_WRITE_SYSTEM_FAIL		0xC2
#define STPV2_RESP_WRITE_TAG_FAIL 		  0xC4

#define STPV2_RESP_NON_ASCII_CHARACTER_IN_REQUEST		0x90
#define STPV2_RESP_BAD_CRC 													0x81
#define STPV2_RESP_FLAGS_DO_NOT_MATCH_COMMAND				0x82
#define STPV2_RESP_FLAGS_DO_NOT_MATCH_TAG_TYPE			0x83
#define STPV2_RESP_UNKNOWN_COMMAND 									0x84
#define STPV2_RESP_UNKNOWN_TAG_TYPE 								0x85
#define STPV2_RESP_INVALID_STARTING_BLOCK						0x86
#define STPV2_RESP_INVALID_NUMBER_OF_BLOCKS					0x87
#define STPV2_RESP_INVALID_MESSAGE_LENGTH						0x88


/*System Parameters*/
#define STPV2_SERIAL	 0x00
#define STPV2_FIRMWARE   0x01

/* Lookup table for commands and responses */
typedef struct STPV2_CODE_LOOKUP {
	unsigned char cmd;
	TCHAR *name;
} STPV2_CODE_LOOKUP;

static STPV2_CODE_LOOKUP stpv2Commands[] = {
	{STPV2_CMD_SELECT_TAG, _T("SelectTag")},
	{STPV2_CMD_READ_TAG, _T("ReadTag")},
	{STPV2_CMD_WRITE_TAG, _T("WriteTag")},
	{STPV2_CMD_READ_MEMORY, _T("ReadMemory")},
	{STPV2_CMD_WRITE_MEMORY, _T("WriteMemory")},
	{STPV2_CMD_READ_SYSTEM, _T("ReadSystem")},
	{STPV2_CMD_WRITE_SYSTEM, _T("WriteSystem")}
};

static STPV2_CODE_LOOKUP stpv2Responses[] = {
	{STPV2_RESP_SELECT_TAG_PASS, _T("SelectTag: PASS")},
	{STPV2_RESP_SELECT_TAG_LOOP_ON, _T("SELECT TAG LOOP ON")},
	{STPV2_RESP_SELECT_TAG_FAIL, _T("SelectTag: FAIL")},
	{STPV2_RESP_SELECT_TAG_LOOP_OFF, _T("SELECT TAG LOOP OFF")},
	{STPV2_RESP_READ_MEMORY_PASS, _T("ReadMemory: PASS")},
	{STPV2_RESP_READ_SYSTEM_PASS, _T("ReadSystem: PASS")},
	{STPV2_RESP_READ_TAG_PASS, _T("ReadTag: PASS")},
	{STPV2_RESP_READ_MEMORY_FAIL, _T("ReadMemory: FAIL")},
	{STPV2_RESP_READ_SYSTEM_FAIL, _T("ReadSystem: FAIL")},
	{STPV2_RESP_READ_TAG_FAIL, _T("ReadTag: FAIL")},
	{STPV2_RESP_EVENT_REPORT, _T("EVENT REPORT")},
	{STPV2_RESP_EVENT_FAIL, _T("EVENT FAIL")},
	{STPV2_RESP_WRITE_MEMORY_PASS, _T("WriteMemory: PASS")},
	{STPV2_RESP_WRITE_SYSTEM_PASS, _T("WriteSystem: PASS")},
	{STPV2_RESP_WRITE_TAG_PASS, _T("WriteTag: PASS")},
	{STPV2_RESP_WRITE_MEMORY_FAIL, _T("WriteMemory: FAIL")},
	{STPV2_RESP_WRITE_SYSTEM_FAIL, _T("WriteSystem: FAIL")},
	{STPV2_RESP_WRITE_TAG_FAIL, _T("WriteTag: FAIL")},
	{STPV2_RESP_BAD_CRC, _T("BAD CRC")},
	{STPV2_RESP_FLAGS_DO_NOT_MATCH_COMMAND, _T("FLAGS DO NOT MATCH COMMAND")},
	{STPV2_RESP_FLAGS_DO_NOT_MATCH_TAG_TYPE, _T("FLAGS DO NOT MATCH TAG TYPE")},
	{STPV2_RESP_UNKNOWN_COMMAND, _T("UNKNOWN COMMAND")},
	{STPV2_RESP_UNKNOWN_TAG_TYPE, _T("UNKOWN TAG TYPE")},
	{STPV2_RESP_INVALID_STARTING_BLOCK, _T("INVALID STARTING BLOCK")},
	{STPV2_RESP_INVALID_NUMBER_OF_BLOCKS, _T("INVALID NUMBER OF BLOCKS")},
	{STPV2_RESP_INVALID_MESSAGE_LENGTH, _T("INVALID MESSAGE LENGTH")}
};

#define STPV2_CMD_LOOKUPS_COUNT sizeof(stpv2Commands)/sizeof(STPV2_CODE_LOOKUP) 
#define STPV2_RESP_LOOKUPS_COUNT sizeof(stpv2Responses)/sizeof(STPV2_CODE_LOOKUP) 

typedef struct STPV2_STATUS_LOOKUP {
	unsigned char cmd;
	SKYETEK_STATUS status;
} STPV2_STATUS_LOOKUP;

static STPV2_STATUS_LOOKUP stpv2Statuses[] = {
	{STPV2_RESP_SELECT_TAG_PASS, SKYETEK_SUCCESS},
	{STPV2_RESP_SELECT_TAG_FAIL, SKYETEK_FAILURE},
	{STPV2_RESP_READ_MEMORY_PASS, SKYETEK_SUCCESS},
	{STPV2_RESP_READ_MEMORY_FAIL, SKYETEK_FAILURE},
	{STPV2_RESP_READ_SYSTEM_PASS, SKYETEK_SUCCESS},
	{STPV2_RESP_READ_SYSTEM_FAIL, SKYETEK_FAILURE},
	{STPV2_RESP_READ_TAG_PASS, SKYETEK_SUCCESS},
	{STPV2_RESP_READ_TAG_FAIL, SKYETEK_FAILURE},
	{STPV2_RESP_WRITE_MEMORY_PASS, SKYETEK_SUCCESS},
	{STPV2_RESP_WRITE_MEMORY_FAIL, SKYETEK_FAILURE},
	{STPV2_RESP_WRITE_SYSTEM_PASS, SKYETEK_SUCCESS},
	{STPV2_RESP_WRITE_SYSTEM_FAIL, SKYETEK_FAILURE},
	{STPV2_RESP_WRITE_TAG_PASS, SKYETEK_SUCCESS},
	{STPV2_RESP_WRITE_TAG_FAIL, SKYETEK_FAILURE},

	{STPV2_RESP_NON_ASCII_CHARACTER_IN_REQUEST, SKYETEK_INVALID_ASCII_CHAR},
	{STPV2_RESP_BAD_CRC, SKYETEK_INVALID_CRC},
	{STPV2_RESP_FLAGS_DO_NOT_MATCH_COMMAND, SKYETEK_INVALID_FLAGS},
	{STPV2_RESP_FLAGS_DO_NOT_MATCH_TAG_TYPE, SKYETEK_INVALID_FLAGS},
	{STPV2_RESP_UNKNOWN_COMMAND, SKYETEK_INVALID_COMMAND},
  {STPV2_RESP_UNKNOWN_TAG_TYPE, SKYETEK_INVALID_TAG_TYPE},
	{STPV2_RESP_INVALID_STARTING_BLOCK, SKYETEK_INVALID_ADDRESS},
	{STPV2_RESP_INVALID_NUMBER_OF_BLOCKS, SKYETEK_INVALID_NUMBER_OF_BLOCKS},
	{STPV2_RESP_INVALID_MESSAGE_LENGTH, SKYETEK_INVALID_MESSAGE_LENGTH}
};

#define STPV2_STATUS_LOOKUPS_COUNT sizeof(stpv2Statuses)/sizeof(STPV2_STATUS_LOOKUP) 


SKYETEK_STATUS 
STPV2_SendCommand(
    LPSKYETEK_READER     lpReader,
    unsigned char         cmd,
    unsigned char         extraFlags,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_SendSetCommand(
    LPSKYETEK_READER     lpReader,
    unsigned char        cmd,
    unsigned char        extraFlags,
    LPSKYETEK_DATA       lpData,
    unsigned int         retries
    );

SKYETEK_STATUS 
STPV2_SendAddrGetCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       *lpData,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_SendAddrSetCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpData,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_SendTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned char         cmd,
    unsigned char         extraFlags,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_SendGetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       *lpData,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_SendSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpData,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_SendGetSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_SendAddrGetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       *lpData,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_SendAddrSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpData,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_SendAddrGetSetTagCommand(
    LPSKYETEK_READER     lpReader,
    LPSKYETEK_TAG        lpTag,
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned char         cmd,
    unsigned char         extraFlags,
    LPSKYETEK_DATA       lpSendData,
    LPSKYETEK_DATA       *lpRecvData,
    unsigned int          retries
    );

SKYETEK_STATUS 
STPV2_GetStatus(
  unsigned char code
  );

unsigned char 
STPV2_IsErrorResponse(
  unsigned short code
  );


#ifdef __cplusplus
}
#endif

#endif 
