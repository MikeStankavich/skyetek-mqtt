/**
 * Protocol.h
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Protocol implementation.
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../SkyeTekAPI.h"

typedef struct PROTOCOL_FLAGS
{
  unsigned char isInventory;
  unsigned char isLoop;
  unsigned char isEncrypt;
  unsigned char isHMAC;
  unsigned char isLock;
} PROTOCOL_FLAGS, *LPPROTOCOL_FLAGS;

typedef struct TAGTYPE_ARRAY
{
  SKYETEK_TAGTYPE type;
} TAGTYPE_ARRAY, *LPTAGTYPE_ARRAY;

/**
 * Tag select callback used by inventory and loop modes.
 * Note that the callback function should copy any memory
 * from lpData before returning; it will be freed by the caller.
 * @param type Tag type
 * @param lpData Data pointer
 * @param user User data
 * @return 0 to stop inventory/loop, 1 to continue
 */ 
typedef unsigned char 
(*PROTOCOL_TAG_SELECT_CALLBACK)(
    SKYETEK_TAGTYPE type,
    LPSKYETEK_DATA lpData,
    void  *user
    );

typedef struct PROTOCOLIMPL 
{
  /** Protocol version */
  unsigned char version;

  /* Reader Functions */
  SKYETEK_STATUS 
  (*SelectTags)(
    LPSKYETEK_READER             lpReader, 
    SKYETEK_TAGTYPE              tagType, 
    PROTOCOL_TAG_SELECT_CALLBACK callback,
    PROTOCOL_FLAGS               flags,
    void                         *user,
    unsigned int                 timeout
    );

  SKYETEK_STATUS 
  (*GetTags)(
    LPSKYETEK_READER   lpReader, 
    SKYETEK_TAGTYPE    tagType, 
    LPTAGTYPE_ARRAY    **tagTypes, 
    LPSKYETEK_DATA     **lpData, 
    unsigned int       *count,
    unsigned int       timeout
    );

  SKYETEK_STATUS 
  (*GetTagsWithMask)(
    LPSKYETEK_READER   lpReader, 
    SKYETEK_TAGTYPE    tagType, 
	LPSKYETEK_ID	   lpTagIdMask,
    LPTAGTYPE_ARRAY    **tagTypes, 
    LPSKYETEK_DATA     **lpData, 
    unsigned int       *count,
    unsigned int       timeout
    );

  SKYETEK_STATUS 
  (*StoreKey)(
    LPSKYETEK_READER     lpReader,
    SKYETEK_TAGTYPE      type,
    LPSKYETEK_ADDRESS    lpAddr,
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    );

  SKYETEK_STATUS 
  (*LoadKey)(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_ADDRESS    lpAddr,
    unsigned int         timeout
    );


  SKYETEK_STATUS 
  (*LoadDefaults)(
    LPSKYETEK_READER     lpReader,
    unsigned int         timeout
    );

  SKYETEK_STATUS 
  (*ResetDevice)(
    LPSKYETEK_READER     lpReader,
    unsigned int         timeout
    );

  SKYETEK_STATUS 
  (*Bootload)(
    LPSKYETEK_READER     lpReader,
    unsigned int         timeout
    );

  SKYETEK_STATUS 
  (*GetSystemParameter)(
    LPSKYETEK_READER             lpReader, 
    LPSKYETEK_ADDRESS            lpAddr,
    LPSKYETEK_DATA               *lpData,
    unsigned int                 timeout
    );

  SKYETEK_STATUS 
  (*SetSystemParameter)(
    LPSKYETEK_READER             lpReader, 
    LPSKYETEK_ADDRESS            lpAddr,
    LPSKYETEK_DATA               lpData,
    unsigned int                 timeout
    );

  SKYETEK_STATUS 
  (*GetDefaultSystemParameter)(
    LPSKYETEK_READER             lpReader, 
    LPSKYETEK_ADDRESS            lpAddr,
    LPSKYETEK_DATA               *lpData,
    unsigned int                 timeout
    );

  SKYETEK_STATUS 
  (*SetDefaultSystemParameter)(
    LPSKYETEK_READER             lpReader, 
    LPSKYETEK_ADDRESS            lpAddr,
    LPSKYETEK_DATA               lpData,
    unsigned int                 timeout
    );

  SKYETEK_STATUS 
  (*AuthenticateReader)(
    LPSKYETEK_READER     lpReader, 
    LPSKYETEK_DATA       lpData,
    unsigned int         timeout
    );

  SKYETEK_STATUS 
  (*EnableDebug)(
    LPSKYETEK_READER     lpReader,
    unsigned int         timeout
    );

  SKYETEK_STATUS 
  (*DisableDebug)(
    LPSKYETEK_READER     lpReader,
    unsigned int         timeout
    );

  SKYETEK_STATUS 
  (*UploadFirmware)(
    LPSKYETEK_READER                      lpReader,
    TCHAR                                  *file, 
    unsigned char                         defaultsOnly,
    SKYETEK_FIRMWARE_UPLOAD_CALLBACK      callback, 
    void                                  *user
    );

  SKYETEK_STATUS 
  (*EnterPaymentScanMode)(
    LPSKYETEK_READER     lpReader,
    unsigned int timeout
    );

  SKYETEK_STATUS 
  (*ScanPayments)(
    LPSKYETEK_READER              lpReader,
    SKYETEK_PAYMENT_SCAN_CALLBACK callback,
    void                          *user,
    unsigned int                  timeout
    );

  /* Tag functions */
  SKYETEK_STATUS 
  (*SelectTag)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ReadTagData)(    
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      PROTOCOL_FLAGS       flags,
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       *lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*WriteTagData)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      PROTOCOL_FLAGS       flags,
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ReadTagConfig)(    
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      PROTOCOL_FLAGS       flags,
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       *lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*WriteTagConfig)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      PROTOCOL_FLAGS       flags,
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ActivateTagType)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*DeactivateTagType)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*SetTagBitRate)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*GetTagInfo)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       *lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*GetLockStatus)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       *lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*KillTag)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ReviveTag)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*EraseTag)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag, 
      unsigned int         timeout 
      );

  SKYETEK_STATUS 
  (*FormatTag)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*DeselectTag)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*AuthenticateTag)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*SendTagPassword)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*GetApplicationIDs)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       *lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*SelectApplication)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*CreateApplication)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*DeleteApplication)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*GetFileIDs)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*SelectFile)(
      LPSKYETEK_READER    lpReader,
      LPSKYETEK_TAG       lpTag,
      LPSKYETEK_DATA      lpData,
      unsigned int        timeout
      );

  SKYETEK_STATUS 
  (*CreateFile)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*GetFileSettings)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpSendData,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ChangeFileSettings)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpSendData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ReadFile)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       lpSendData,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*WriteFile)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*DeleteFile)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ClearFile)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*CreditValueFile)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );


  SKYETEK_STATUS 
  (*DebitValueFile)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*LimitedCreditValueFile)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*GetValue)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       lpSendData,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ReadRecords)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       lpSendData,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );
    	  
  SKYETEK_STATUS 
  (*WriteRecord)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_ADDRESS    lpAddr,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*CommitTransaction)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*AbortTransaction)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*EnableEAS)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*DisableEAS)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ScanEAS)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ReadAFI)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       *lpData,
      unsigned int         timeout
      );
    	  
  SKYETEK_STATUS 
  (*WriteAFI)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ReadDSFID)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       *lpData,
      unsigned int         timeout
      );
    	  
  SKYETEK_STATUS 
  (*WriteDSFID)(
      LPSKYETEK_READER     lpReader, 
      LPSKYETEK_TAG        lpTag, 
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*GetKeyVersion)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpSendData,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ChangeKey)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*GetKeySettings)(
      LPSKYETEK_READER        lpReader,
      LPSKYETEK_TAG           lpTag,
      LPSKYETEK_DATA          *lpRecvData,
      unsigned int            timeout
      );

  SKYETEK_STATUS 
  (*ChangeKeySettings)(
      LPSKYETEK_READER             lpReader,
      LPSKYETEK_TAG                lpTag,
      LPSKYETEK_DATA               lpData,
      unsigned int                 timeout
      );

  SKYETEK_STATUS 
  (*InitializeSecureMemoryTag)(
      LPSKYETEK_READER          lpReader,
      LPSKYETEK_TAG             lpTag,
      LPSKYETEK_DATA            lpData,
      unsigned int              timeout
      );

  SKYETEK_STATUS 
  (*SetupSecureMemoryTag)(
      LPSKYETEK_READER          lpReader,
      LPSKYETEK_TAG             lpTag,
      LPSKYETEK_DATA            lpData,
      unsigned int              timeout
      );

  SKYETEK_STATUS 
  (*InterfaceSend)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpSendData,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*TransportSend)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpSendData,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*InitiatePayment)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*ComputePayment)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_TAG        lpTag,
      LPSKYETEK_DATA       lpSendData,
      LPSKYETEK_DATA       *lpRecvData,
      unsigned int         timeout
      );

  SKYETEK_STATUS 
  (*GetDebugMessages)(
      LPSKYETEK_READER     lpReader,
      LPSKYETEK_DATA       *lpData,
      unsigned int         timeout
      );

} PROTOCOLIMPL, *LPPROTOCOLIMPL;


extern PROTOCOLIMPL STPV2Impl;
extern PROTOCOLIMPL STPV3Impl;


typedef struct SYSPARMDESC 
{
  SKYETEK_SYSTEM_PARAMETER  parameter;
  TCHAR                      *name;
} SYSPARMDESC, *LPSYSPARMDESC;


static SYSPARMDESC SysParmIds [] = {
  {SYS_SERIALNUMBER    , _T("Serial Number")}, /* 0x0000, 4 bytes */
  {SYS_FIRMWARE        , _T("Firmware Version")}, /* 0x0001,  4 bytes */
  {SYS_HARDWARE        , _T("Hardware Version")}, /* 0x0002,  4 bytes */
  {SYS_PRODUCT         , _T("Product Code")}, /* 0x0003,  2 bytes */
  {SYS_RID             , _T("Reader ID")}, /* 0x0004,  4 bytes */
  {SYS_READER_NAME     , _T("Reader Name")}, /* 0x0005,  32 bytes */
  {SYS_HOST_INTERFACE  , _T("Host Interface Type")}, /* 0x0006,  1 byte */
  {SYS_BAUD            , _T("Host Interface Baud Rate")}, /* 0x0007,  1 byte */
  {SYS_PORT_DIRECTION  , _T("User Port Direction")}, /* 0x0008,  1 byte */
  {SYS_PORT_VALUE      , _T("User Port Value")}, /* 0x0009,  1 byte */
  {SYS_MUX_CONTROL     , _T("MUX Control")}, /* 0x000A,  1 byte */
  {SYS_BOOTLOAD        , _T("Bootload Mode")}, /* 0x000B,  1 byte */
  {SYS_OPERATING_MODE  , _T("Operating Mode")}, /* 0x000C,  1 byte */
  /*{SYS_ENCRYPTION      , _T("Encryption Scheme")},  0x000D,  1 byte */
  /*{SYS_HMAC            , _T("HMAC Scheme")},  0x000E,  1 byte */
  {SYS_PERIPHERAL      , _T("Peripheral Selection")}, /* 0x000F,  1 byte */
  {SYS_TAG_POPULATION  , _T("Tag Population Optimization")}, /* 0x0010,  1 byte */
  {SYS_COMMAND_RETRY   , _T("Command Retry")}, /* 0x0011,  1 byte */
  {SYS_POWER_LEVEL     , _T("Power Level")}, /* 0x0012,  1 byte */

  /* M2 Specific Parameters */
  {SYS_RECV_SENSITIVITY  , _T("Receiver Sensitivity")}, /* 1 byte */
  {SYS_DESFIRE_FRAMING   , _T("DESFire Framing Format")}, /* 1 byte */  
  {SYS_TR_DATA_RATE      , _T("Max T-R Data Rate")}, /* 1 byte */
  {SYS_RT_DATA_RATE      , _T("Max R-T Data Rate")}, /* 1 byte */
  
  /* M9 Specific System Parameters */
  {SYS_CURRENT_FREQUENCY     , _T("Current Frequency")}, /* 4 bytes */  
  {SYS_START_FREQUENCY       , _T("Start Frequency")}, /* 4 bytes */  
  {SYS_STOP_FREQUENCY        , _T("Stop Frequency")}, /* 4 bytes */  
  {SYS_RADIO_LOCK			 , _T("Radio Lock")}, /* 1 byte */
  {SYS_HOP_CHANNEL_SPACING   , _T("Hop Channel Spacing")}, /* 4 bytes */
  {SYS_FREQ_HOP_SEQ          , _T("Frequency Hopping Sequence")}, /* 1 byte */
  {SYS_MODULATION_DEPTH      , _T("Modulation Depth")}, /* 1 byte */
  {SYS_REGULATORY_MODE       , _T("Regulatory Mode")}, /* 1 byte */
  {SYS_LISTEN_BEFORE_TALK    , _T("LBT Antenna Gain")}, /*  1 byte */
  /*{SYS_SYNTH_POWER_LEVEL     , "Synthesizer Power Level"},  1 byte */
  /*{SYS_MARK_DAC_VALUE        , "Mark DAC Value"},  2 bytes */
  /*{SYS_SPACE_DAC_VALUE       , "Space DAC Value"},  2 bytes */
  /*{SYS_POWER_DETECTOR_VALUE  , "Power Detector Value"} 2 bytes */
  {SYS_TEST_MODE       , _T("Test Mode")} /* 1 byte */
}; 

#define NUM_SYSPARMDESCS sizeof(SysParmIds)/sizeof(SYSPARMDESC)


#ifdef __cplusplus
}
#endif

#endif
