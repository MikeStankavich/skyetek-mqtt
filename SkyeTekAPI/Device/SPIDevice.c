/**
 * SPIDevice.c
 * Copyright � 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the SPIDevice.c
 */

#include "../SkyeTekAPI.h"
#include "Device.h"
#include "SPIDevice.h"
#include <stdlib.h>
#ifdef LINUX
#include <malloc.h>
#endif

SKYETEK_STATUS 
SPIDevice_Open(LPSKYETEK_DEVICE device)
{
  LPSPI_INFO info;

	if( device == NULL || device->user == NULL )
		return SKYETEK_INVALID_PARAMETER;

  if( device->readFD != 0 && device->writeFD != 0 )
	  return SKYETEK_SUCCESS;

  info = (LPSPI_INFO)device->user;
  info->spiHandle = aa_open(info->port_number);

  if( info->spiHandle < 1 )
  {
    device->readFD = device->writeFD = 0;
    return SKYETEK_FAILURE;
  }

  if( info->type == AA_FEATURE_I2C )
  {
    if( 0 > aa_configure(info->spiHandle, AA_CONFIG_GPIO_I2C) )
      goto failure;
    aa_i2c_pullup(info->spiHandle, AA_I2C_PULLUP_BOTH);
    aa_i2c_bitrate(info->spiHandle, 400);
  }
  else
  {
    /* Set communication parameters */
    if( 0 > aa_configure(info->spiHandle, AA_CONFIG_SPI_GPIO) )
      goto failure;
    if( AA_OK != aa_spi_configure(info->spiHandle,AA_SPI_POL_FALLING_RISING,AA_SPI_PHASE_SETUP_SAMPLE,AA_SPI_BITORDER_MSB) )
      goto failure;
    if( 0 > aa_spi_bitrate(info->spiHandle, 400) ) /* 400 kHz */
      goto failure;
    if( AA_OK != aa_spi_master_ss_polarity(info->spiHandle, AA_SPI_SS_ACTIVE_LOW) )
      goto failure;
    if( AA_OK != aa_gpio_direction(info->spiHandle, 0x00) )
      goto failure;
    if( 0 > aa_gpio_pullup(info->spiHandle, 0x02) )
      goto failure;
  }


  device->readFD = (SKYETEK_DEVICE_FILE)1;
  device->writeFD = (SKYETEK_DEVICE_FILE)1;
	return SKYETEK_SUCCESS;
failure:
  aa_close(info->spiHandle);
  return SKYETEK_FAILURE;
}

SKYETEK_STATUS 
SPIDevice_Close(LPSKYETEK_DEVICE device)
{
  LPSPI_INFO info;
	if(device == NULL || device->user == NULL)
		return SKYETEK_INVALID_PARAMETER;
	
  info = (LPSPI_INFO)device->user;
  if( info->spiHandle < 1 )
    return SKYETEK_INVALID_PARAMETER;

  aa_close(info->spiHandle);  

	device->readFD = device->writeFD = 0;
	return SKYETEK_SUCCESS;
}

int 
SPIDevice_Write(LPSKYETEK_DEVICE device,
		unsigned char* buffer,
		unsigned int length,
    unsigned int timeout)
{
  LPSPI_INFO info;
  int bytes = 0;
  unsigned char resp;
  unsigned int i;
  aa_u16 len = (aa_u16)length;

	if((device == NULL) || (buffer == NULL) || (device->user == NULL) )
		return 0;

  if( length == 0 )
    return 0;

  info = (LPSPI_INFO)device->user;
  if( info->spiHandle < 1 )
    return 0;

	MUTEX_LOCK(&info->lock);
  if( info->type == AA_FEATURE_I2C )
  {
    bytes = aa_i2c_write(info->spiHandle, 0x007F, 0, len, buffer);
  }
  else
  {
    for( i = 0; i < length; i++ )
      bytes += aa_spi_write(info->spiHandle, 1, &buffer[i], &resp);
  }
	MUTEX_UNLOCK(&info->lock);
  return bytes;
}

int 
SPIDevice_Read(LPSKYETEK_DEVICE device,
		unsigned char* buffer,
		unsigned int length,
    unsigned int timeout
    )
{
  LPSPI_INFO info;
	int bytes = 0;
  unsigned char req = 0x00;
  unsigned int i;
  unsigned char bit;
  aa_u16 len = (aa_u16)length;

  if( (device == NULL) || (buffer == NULL) || (device->user == NULL) )
		return 0;
  if( length == 0 )
    return 0;

  info = (LPSPI_INFO)device->user;
  if( info->spiHandle < 1 )
    return SKYETEK_INVALID_PARAMETER;

  SKYETEK_Sleep(100);

	MUTEX_LOCK(&info->lock);
  if( info->type == AA_FEATURE_I2C )
  {
    bytes = aa_i2c_read(info->spiHandle, 0x007F, 0, len, buffer);
  }
  else
  {
    for( i = 0; i < length; i++ )
    {
      bit = aa_gpio_get(info->spiHandle);
      if( bit & 0x01 )
        bytes += aa_spi_write(info->spiHandle, 1, &req, &buffer[i]);
      else
        SKYETEK_Sleep(10);
    }
  }
  MUTEX_UNLOCK(&info->lock);
  return bytes;
}

void 
SPIDevice_Flush(LPSKYETEK_DEVICE device)
{
  return;
}

int 
SPIDevice_Free(LPSKYETEK_DEVICE device)
{
	LPSPI_INFO info;

  if( device == NULL || device->user == NULL )
    return 0;

  info = (LPSPI_INFO)device->user;
  aa_close(info->spiHandle);  
  MUTEX_DESTROY(&info->lock);
  free(info);
  device->user = NULL;

	return 1;
}

SKYETEK_STATUS
SPIDevice_SetAdditionalTimeout(
  LPSKYETEK_DEVICE  lpDevice,
  unsigned int      timeout
  )
{
  return SKYETEK_SUCCESS;
}

void 
SPIDevice_InitDevice(LPSKYETEK_DEVICE device)
{
	LPSPI_INFO info;
  TCHAR *ptr;
  
	if( device == NULL )
		return;

  ptr = device->address;
  ptr++; ptr++; ptr++; /* skip SPI */

  info = (LPSPI_INFO)malloc(sizeof(SPI_INFO));
  memset(info,0,sizeof(SPI_INFO));
  info->port_number = _ttoi(ptr);
  if( _tcsstr(device->address,SKYETEK_I2C_DEVICE_TYPE) != NULL )
    info->type = AA_FEATURE_I2C;
  else
    info->type = AA_FEATURE_SPI;

  MUTEX_CREATE(&info->lock);

	device->internal = &SPIDeviceImpl;
	device->user = (void*)info;
}

DEVICEIMPL SPIDeviceImpl = {
  SPIDevice_Open,
	SPIDevice_Close,
	SPIDevice_Read,
	SPIDevice_Write,
	SPIDevice_Flush,
	SPIDevice_Free,
  SPIDevice_SetAdditionalTimeout,
  0
};
