/**
 * SerialDevice.c
 * Copyright © 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the SerialDevice.c
 */

#include "../SkyeTekAPI.h"
#include "Device.h"
#include "SerialDevice.h"
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#ifndef WIN32
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __sun
#include <sys/mkdev.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>
#include <errno.h>
#endif

#include "../Platform.h"


#if defined(WIN32)
	#define SERIAL_CLOSE(h) CloseHandle(h)
	#define SERIAL_READ(h, b, l, r) ReadFile(h, b, l, &r, NULL)
	#define SERIAL_WRITE(h, b, l, w) WriteFile(h, b, l, &w, NULL)
#else
	#define SERIAL_CLOSE(h) close(h)
	#define SERIAL_READ(h, b, l, r) r = read(h, b, l)
	#define SERIAL_WRITE(h, b, l, w) w = write(h, b, l)
#endif

#ifdef WIN32
SKYETEK_STATUS 
SerialDevice_Open(
  LPSKYETEK_DEVICE device
  )
{
#if !defined(WINCE)
  COMMTIMEOUTS ctos;
  LPDEVICEIMPL di;
#endif

	if( device == NULL || device->internal == NULL )
		return SKYETEK_INVALID_PARAMETER;
  if( device->readFD != 0 && device->writeFD != 0 )
    return SKYETEK_SUCCESS;

  device->readFD = device->writeFD = CreateFile(
      device->address,
      GENERIC_READ|GENERIC_WRITE,
      0,NULL,
      OPEN_EXISTING,
      FILE_FLAG_OVERLAPPED,
      NULL);
	if( device->readFD == INVALID_HANDLE_VALUE )
	{
		device->readFD = device->writeFD = 0;
		return SKYETEK_READER_IO_ERROR;
	}
	
#if !defined(WINCE)
	/* Global timeouts not used, instead overlapped wait used */
  di = (LPDEVICEIMPL)device->internal;
  ZeroMemory(&ctos,sizeof(COMMTIMEOUTS));
	SetCommTimeouts(device->readFD, &ctos); 

  PurgeComm( device->readFD, PURGE_TXCLEAR | PURGE_TXABORT );
  PurgeComm( device->readFD, PURGE_RXCLEAR | PURGE_RXABORT );

#endif 

	return SKYETEK_SUCCESS;
}

SKYETEK_API SKYETEK_STATUS 
SerialDevice_SetOptions(
  LPSKYETEK_DEVICE            device, 
  LPSKYETEK_SERIAL_SETTINGS   lpSettings
  )
{
	DCB dcb;

	if( device == NULL || lpSettings == NULL )
		return SKYETEK_INVALID_PARAMETER;

	if( device->readFD == 0 )
		return SKYETEK_FAILURE;

	/* Set up baud rate, etc. */
	FillMemory(&dcb, sizeof(dcb), 0);
	dcb.DCBlength = sizeof(dcb);

  switch(lpSettings->baudRate)
  {
  case 110:
		dcb.BaudRate = CBR_110;
    break;
	case 300:
		dcb.BaudRate = CBR_300;
    break;
	case 600:
		dcb.BaudRate = CBR_600;
    break;
	case 1200:
		dcb.BaudRate = CBR_1200;
    break;
	case 2400:
		dcb.BaudRate = CBR_2400;
    break;
	case 4800:
		dcb.BaudRate = CBR_4800;
    break;
	case 9600:
		dcb.BaudRate = CBR_9600;
    break;
	case 14400:
		dcb.BaudRate = CBR_14400;
    break;
	case 19200:
		dcb.BaudRate = CBR_19200;
    break;
	case 38400:
		dcb.BaudRate = CBR_38400;
    break;
	case 56000:
		dcb.BaudRate = CBR_56000;
    break;
	case 57600:
		dcb.BaudRate = CBR_57600;
    break;
	case 115200:
		dcb.BaudRate = CBR_115200;
    break;
	case 128000:
		dcb.BaudRate = CBR_128000;
    break;
	case 256000:
		dcb.BaudRate = CBR_256000;
    break;
  default:
		return SKYETEK_INVALID_PARAMETER;
  }

	if( lpSettings->dataBits < 1 || lpSettings->dataBits > 16 )
		return SKYETEK_INVALID_PARAMETER;
	dcb.ByteSize = lpSettings->dataBits;

	switch(lpSettings->parity)
	{
		case MARK:
			dcb.fParity = MARKPARITY;
			break;
		case EVEN:
			dcb.fParity = EVENPARITY;
			break;
		case ODD:
			dcb.fParity = ODDPARITY;
			break;
		case SPACE:
			dcb.fParity = SPACEPARITY;
			break;
		case NONE:
			dcb.fParity = NOPARITY;
			break;
		default:
			return SKYETEK_INVALID_PARAMETER;
	}

	switch(lpSettings->stopBits)
	{
		case ONE:
			dcb.StopBits = ONESTOPBIT;
			break;
		case ONE_POINT_FIVE:
			dcb.StopBits = ONE5STOPBITS;
			break;
		case TWO:
			dcb.StopBits = TWOSTOPBITS;
			break;
		default:
			return SKYETEK_INVALID_PARAMETER;
	}

	if( SetCommState(device->readFD, &dcb) )
		return SKYETEK_SUCCESS;
	else
		return SKYETEK_FAILURE;
}

SKYETEK_STATUS 
SerialDevice_GetOptions(
  LPSKYETEK_DEVICE            device, 
  LPSKYETEK_SERIAL_SETTINGS   lpSettings
  )
{
	DCB dcb;

	if( device == NULL )
		return SKYETEK_INVALID_PARAMETER;

	if( device->readFD == 0 )
		return SKYETEK_FAILURE;

	/* Set up baud rate, etc. */
	FillMemory(&dcb, sizeof(dcb), 0);
	dcb.DCBlength = sizeof(dcb);

	if( !GetCommState(device->readFD, &dcb) )
		return SKYETEK_FAILURE;

  switch(dcb.BaudRate)
  {
  case CBR_110:
		lpSettings->baudRate = 110;
    break;
	case CBR_300:
		lpSettings->baudRate = 300;
    break;
	case CBR_600:
		lpSettings->baudRate = 600;
    break;
	case CBR_1200:
		lpSettings->baudRate = 1200;
    break;
	case CBR_2400:
		lpSettings->baudRate = 2400;
    break;
	case CBR_4800:
		lpSettings->baudRate = 4800;
    break;
	case CBR_9600:
		lpSettings->baudRate = 9600;
    break;
	case CBR_14400:
		lpSettings->baudRate = 14400;
    break;
	case CBR_19200:
		lpSettings->baudRate = 19200;
    break;
	case CBR_38400:
		lpSettings->baudRate = 38400;
    break;
	case CBR_56000:
		lpSettings->baudRate = 56000;
    break;
	case CBR_57600:
		lpSettings->baudRate = 57600;
    break;
	case CBR_115200:
		lpSettings->baudRate = 115200;
    break;
	case CBR_128000:
		lpSettings->baudRate = 128000;
    break;
	case CBR_256000:
		lpSettings->baudRate = 256000;
    break;
  default:
		return SKYETEK_FAILURE;
  }

	lpSettings->dataBits = dcb.ByteSize;

	switch(dcb.fParity )
	{
		case MARKPARITY:
			lpSettings->parity = MARK;
			break;
		case EVENPARITY:
			lpSettings->parity = EVEN;
			break;
		case ODDPARITY:
			lpSettings->parity = ODD;
			break;
		case SPACEPARITY:
			lpSettings->parity = SPACE;
			break;
		case NOPARITY:
			lpSettings->parity = NONE;
			break;
		default:
			return SKYETEK_FAILURE;
	}

	switch(dcb.StopBits)
	{
		case ONESTOPBIT:
			lpSettings->stopBits = ONE;
			break;
		case ONE5STOPBITS:
			lpSettings->stopBits = ONE_POINT_FIVE;
			break;
		case TWOSTOPBITS:
			lpSettings->stopBits = TWO;
			break;
		default:
			return SKYETEK_FAILURE;
	}
  return SKYETEK_SUCCESS;
}


#else

void
SetTermiosOptions(int fd)
{
	struct termios options;
	
	tcgetattr(fd, &options);

  memset(&options, 0, sizeof(struct termios));
  options.c_cflag = CS8 | CLOCAL | CREAD;
  options.c_cflag |= B38400;
  options.c_iflag = IGNPAR | IGNBRK;
  options.c_oflag = 0;
  options.c_lflag = 0;
  options.c_cc[VTIME] = 1;
  options.c_cc[VMIN] = 1;
  
	tcflush(fd, TCIFLUSH);	
	tcsetattr(fd, TCSANOW, &options);
}

SKYETEK_STATUS
SerialDevice_Open(
  LPSKYETEK_DEVICE device
  )
{
	struct stat deviceStat;
	unsigned int major;

	if( device == NULL )
		return SKYETEK_INVALID_PARAMETER;
  if( device->readFD != 0 && device->writeFD != 0 )
    return SKYETEK_SUCCESS;
	
	if(stat(device->address, &deviceStat) == -1)
		return SKYETEK_INVALID_PARAMETER;

	major = major(deviceStat.st_rdev);
	
	switch(major)
	{
#ifdef LINUX
		/* 4 is standard linux serial
		 * 204 is S3C2410 serial
		 * 188 is USB to serial
		 */
		case 4:
		case 188:
		case 204:
#elif defined(__sun)
    case 20:
#else
#error Need to define platform specific major numbers
#endif
			if((device->writeFD = open(device->address,
                                 O_NOCTTY |
                                 O_RDWR |
                                 O_NONBLOCK)) == -1)
				return SKYETEK_READER_IO_ERROR;
			
			SetTermiosOptions(device->writeFD);
			
			device->readFD = device->writeFD;
			device->asynchronous = 1;
			break;
		/*
		 * 89 is I2C
		 * 153 is S3C2410 SPI
		 */
		case 89:
		case 153:
			if((device->writeFD = open(device->address,
                                 O_NOCTTY | O_WRONLY | O_NONBLOCK)) == -1)
				return SKYETEK_READER_IO_ERROR;

			if((device->readFD = open(device->address,
                                O_NOCTTY | O_RDONLY | O_NONBLOCK)) == -1)
			{
				close(device->writeFD);
				return SKYETEK_READER_IO_ERROR;
				
			}
		default:
			return SKYETEK_INVALID_PARAMETER;

	}

	return SKYETEK_SUCCESS;
}

SerialDevice_SetOptionsImpl(int fd, LPSKYETEK_SERIAL_SETTINGS lpSettings)
{
	struct termios options;

	if( fd == 0 || lpSettings == NULL )
		return SKYETEK_INVALID_PARAMETER;

	tcgetattr(fd, &options);

  memset(&options, 0, sizeof(struct termios));
  options.c_cflag = CLOCAL | CREAD;

  switch(lpSettings->baudRate)
  {
  case 110:
    options.c_cflag |= B110;
    break;
	case 300:
    options.c_cflag |= B300;
    break;
	case 600:
    options.c_cflag |= B600;
    break;
	case 1200:
    options.c_cflag |= B1200;
    break;
	case 2400:
    options.c_cflag |= B2400;
    break;
	case 4800:
    options.c_cflag |= B4800;
    break;
	case 9600:
    options.c_cflag |= B9600;
    break;
    /*case 14400:
    options.c_cflag |= B14400;
    break;*/
	case 19200:
    options.c_cflag |= B19200;
    break;
	case 38400:
    options.c_cflag |= B38400;
    break;
    /*case 56000:
    options.c_cflag |= B56000;
    break;*/
	case 57600:
    options.c_cflag |= B57600;
    break;
	case 115200:
    options.c_cflag |= B115200;
    break;
  default:
		return SKYETEK_INVALID_PARAMETER;
  }

  switch(lpSettings->dataBits)
  {
  case 5:
    options.c_cflag |= CS5;
    break;
  case 6:
    options.c_cflag |= CS6;
    break;
  case 7:
    options.c_cflag |= CS7;
    break;
  case 8:
    options.c_cflag |= CS8;
    break;
  default:
    return SKYETEK_INVALID_PARAMETER;
  }

	switch(lpSettings->parity)
	{
	case EVEN:
    options.c_cflag |= PARENB;
		break;
	case ODD:
    options.c_cflag |= PARENB;
    options.c_cflag |= PARODD;
		break;
	case NONE:
		break;
	default:
		return SKYETEK_INVALID_PARAMETER;
	}

	switch(lpSettings->stopBits)
	{
	case ONE:
		break;
	case TWO:
    options.c_cflag |= CSTOPB;
		break;
	default:
		return SKYETEK_INVALID_PARAMETER;
	}

  options.c_iflag = IGNPAR | IGNBRK;
  options.c_oflag = 0;
  options.c_lflag = 0;
  options.c_cc[VTIME] = 1;
  options.c_cc[VMIN] = 1;
  
	tcflush(fd, TCIFLUSH);	
	tcsetattr(fd, TCSANOW, &options);
  return SKYETEK_SUCCESS;
}

SKYETEK_STATUS 
SerialDevice_SetOptions(
  LPSKYETEK_DEVICE            device, 
  LPSKYETEK_SERIAL_SETTINGS   lpSettings
  )
{
  SKYETEK_STATUS st;
  if( (st = SerialDevice_SetOptionsImpl(device->readFD,lpSettings)) == SKYETEK_SUCCESS )
    st = SerialDevice_SetOptionsImpl(device->writeFD,lpSettings);
  return st;
}

SKYETEK_STATUS 
SerialDevice_GetOptions(
  LPSKYETEK_DEVICE            device, 
  LPSKYETEK_SERIAL_SETTINGS   lpSettings
  )
{
	struct termios options;
  int fd = 0;

	if( device->readFD == 0 || lpSettings == NULL )
		return SKYETEK_INVALID_PARAMETER;

  fd = device->readFD;
	tcgetattr(fd, &options);

  if( options.c_cflag & B110 )
    lpSettings->baudRate = 110;
  else if( options.c_cflag & B300 )
    lpSettings->baudRate = 300;
  else if( options.c_cflag & B600 )
    lpSettings->baudRate = 600;
  else if( options.c_cflag & B1200 )
    lpSettings->baudRate = 1200;
  else if( options.c_cflag & B2400 )
    lpSettings->baudRate = 2400;
  else if( options.c_cflag & B4800 )
    lpSettings->baudRate = 4800;
  else if( options.c_cflag & B9600 )
    lpSettings->baudRate = 9600;
  /*else if( options.c_cflag & B14400 )
    lpSettings->baudRate = 14400;*/
  else if( options.c_cflag & B19200 )
    lpSettings->baudRate = 19200;
  else if( options.c_cflag & B38400 )
    lpSettings->baudRate = 38400;
  /*else if( options.c_cflag & B56000 )
    lpSettings->baudRate = 56000;*/
  else if( options.c_cflag & B57600 )
    lpSettings->baudRate = 57600;
  else if( options.c_cflag & B115200 )
    lpSettings->baudRate = 115200;
  else
		lpSettings->baudRate = 0;

  if( options.c_cflag & CS5 )
    lpSettings->dataBits = 5;
  else if( options.c_cflag & CS6 )
    lpSettings->dataBits = 6;
  else if( options.c_cflag & CS7 )
    lpSettings->dataBits = 7;
  else if( options.c_cflag & CS8 )
    lpSettings->dataBits = 8;
  else
    lpSettings->dataBits = 0;

  if( !(options.c_cflag & PARENB) )
    lpSettings->parity = NONE;
  else if( options.c_cflag & PARODD )
    lpSettings->parity = ODD;
  else
    lpSettings->parity = EVEN;

  if( options.c_cflag & CSTOPB )
    lpSettings->stopBits = TWO;
  else
    lpSettings->stopBits = ONE;

  return SKYETEK_SUCCESS;
}

#endif

SKYETEK_STATUS 
SerialDevice_Close(
  LPSKYETEK_DEVICE device
  )
{
	if( device == NULL )
		return SKYETEK_INVALID_PARAMETER;
	
	SERIAL_CLOSE(device->readFD);
	
  	if(device->writeFD != device->readFD)
		SERIAL_CLOSE(device->writeFD);

	device->writeFD = device->readFD = 0;
	return SKYETEK_SUCCESS;
}

int 
SerialDevice_Read(
  LPSKYETEK_DEVICE  device, 
  unsigned char     *buffer, 
  unsigned int      length,
  unsigned int      timeout
  )
{
#if defined(WIN32) && !defined(WINCE)
	OVERLAPPED overlap;
#endif
#ifdef WINCE
	COMMTIMEOUTS ctos;
#endif
  LPDEVICEIMPL di;
	int bytesRead = 0;
  unsigned int to;
	if( (device == NULL) || (buffer == NULL) || (device->internal == NULL) )
		return 0;
  di = (LPDEVICEIMPL)device->internal;
  to = max(di->timeout + timeout,100);

#if defined(WIN32) && !defined(WINCE)
	ZeroMemory(&overlap, sizeof(OVERLAPPED));
	overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ReadFile(device->readFD, buffer, length, (LPDWORD)&bytesRead, &overlap);
	WaitForSingleObject(overlap.hEvent, to);
	CloseHandle(overlap.hEvent);
	if(!HasOverlappedIoCompleted(&overlap))
		CancelIo(device->readFD);
	GetOverlappedResult(device->readFD, &overlap, (LPDWORD)&bytesRead, 0);
#elif defined(WINCE)
  	ZeroMemory(&ctos,sizeof(COMMTIMEOUTS));
	ctos.ReadTotalTimeoutConstant = to;
	SetCommTimeouts(device->readFD, &ctos);
	ReadFile(device->readFD, buffer, length, &bytesRead, NULL);
#else
  if (((bytesRead = read(device->readFD, buffer, length)) == -1) &&
      (errno == EAGAIN)) {
    struct pollfd fds;
    int i;

    fds.fd = device->readFD;
    fds.events = POLLIN;
    fds.revents = 0;

    i = poll(&fds, 1, to);
    if (i <= 0)
      return i;

    bytesRead = read(device->readFD, buffer, length);
  }
#endif
	
	return bytesRead;
}

int 
SerialDevice_Write(
  LPSKYETEK_DEVICE    device, 
  unsigned char       *buffer, 
  unsigned int        length,
  unsigned int        timeout
  )
{
#if defined(WIN32) && !defined(WINCE)
	OVERLAPPED overlap;
#endif
#ifdef WINCE
	COMMTIMEOUTS ctos;
#endif
  LPDEVICEIMPL di;
  int bytesWritten = 0;
  unsigned int to;
	if( (device == NULL) || (buffer == NULL) || (device->internal == NULL) )
		return 0;
  di = (LPDEVICEIMPL)device->internal;
  to = max(timeout + di->timeout,100);

#if defined(WIN32) && !defined(WINCE)
	ZeroMemory(&overlap, sizeof(OVERLAPPED));
	overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	WriteFile(device->writeFD, buffer, length, (LPDWORD)&bytesWritten, &overlap);
	WaitForSingleObject(overlap.hEvent,to);
	CloseHandle(overlap.hEvent);
	if(!HasOverlappedIoCompleted(&overlap))
		CancelIo(device->writeFD);
	GetOverlappedResult(device->writeFD, &overlap, (LPDWORD)&bytesWritten, 0);
#elif defined(WINCE)
	ZeroMemory(&ctos,sizeof(COMMTIMEOUTS));
	ctos.WriteTotalTimeoutConstant = to;
	SetCommTimeouts(device->writeFD, &ctos);
	WriteFile(device->writeFD, buffer, length, &bytesWritten, NULL);
#else
  if (((bytesWritten = write(device->writeFD, buffer, length)) == -1) &&
      (errno == EAGAIN)) {
    struct pollfd fds;
    int i;

    fds.fd = device->writeFD;
    fds.events = POLLOUT;
    fds.revents = 0;

    i = poll(&fds, 1, to);
    if (i <= 0)
      return i;

    bytesWritten = write(device->writeFD, buffer, length);
  }
#endif
	
	return bytesWritten;
}

void 
SerialDevice_Flush(
  LPSKYETEK_DEVICE device
  )
{
	
}

int 
SerialDevice_Free(
  LPSKYETEK_DEVICE device
  )
{
	if(device == NULL)
		return 0;
  SerialDevice_Close(device);
  free(device);
  return 1;
}

SKYETEK_STATUS
SerialDevice_SetAdditionalTimeout(
  LPSKYETEK_DEVICE  lpDevice,
  unsigned int      timeout
  )
{
  LPDEVICEIMPL di;

  if( lpDevice == NULL || lpDevice->internal == NULL )
    return SKYETEK_INVALID_PARAMETER;
  di = (LPDEVICEIMPL)lpDevice->internal;
  di->timeout = timeout;

  return SKYETEK_SUCCESS;
}

void 
SerialDevice_InitDevice(
  LPSKYETEK_DEVICE device
  )
{
	if( device == NULL )
		return;
  device->internal = &SerialDeviceImpl;
}

DEVICEIMPL SerialDeviceImpl = {
  SerialDevice_Open,
	SerialDevice_Close,
	SerialDevice_Read,
	SerialDevice_Write,
	SerialDevice_Flush,
	SerialDevice_Free,
  SerialDevice_SetAdditionalTimeout,
  0
};
