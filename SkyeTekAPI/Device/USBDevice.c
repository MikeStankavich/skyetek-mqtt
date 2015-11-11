/**
 * USBDevice.c
 * Copyright � 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 * Implementation of the USBDevice.c
 */

#include "../SkyeTekAPI.h"
#include "Device.h"
#include "USBDevice.h"
#include <stdlib.h>
#include <malloc.h>

#ifdef HAVE_LIBUSB
#include <usb.h>
#endif

#ifdef LINUX
#include <string.h>
#endif

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif


#if defined(WIN32)
void 
USBDevice_internalFlush(LPSKYETEK_DEVICE device, BOOL lockSendBuffer)
{
	#ifndef WINCE
	OVERLAPPED overlap;
	#else
	COMMTIMEOUTS ctos;
	DEVICEIMPL* pImpl;
	#endif
	unsigned char sendBuffer[65];
	LPUSB_DEVICE usbDevice;
	unsigned int bytesWritten;
	
	if(device == NULL)
		return;

	usbDevice = (LPUSB_DEVICE)device->user;

	if(lockSendBuffer)
		EnterCriticalSection(&usbDevice->sendBufferMutex);

	if(usbDevice->sendBufferWritePtr == usbDevice->sendBuffer)
		goto end;

	/* Windows HID command */
	sendBuffer[0] = 0;
	/* Actual length of data */
	sendBuffer[1] = (usbDevice->sendBufferWritePtr - usbDevice->sendBuffer);
	CopyMemory((sendBuffer + 2), usbDevice->sendBuffer, sendBuffer[1]);

	bytesWritten = 0;
	
#ifndef WINCE
	ZeroMemory(&overlap, sizeof(OVERLAPPED));
	overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	WriteFile(device->writeFD, sendBuffer, 65, &bytesWritten, &overlap);
	
	WaitForSingleObject(overlap.hEvent, 100);

	CloseHandle(overlap.hEvent);
	
	if(!HasOverlappedIoCompleted(&overlap))
		CancelIo(device->writeFD);
#else
	pImpl = (DEVICEIMPL*)device->internal;
	ZeroMemory(&ctos,sizeof(COMMTIMEOUTS));
	ctos.WriteTotalTimeoutConstant = pImpl->timeout;
	SetCommTimeouts(device->writeFD, &ctos);
	WriteFile(device->writeFD, sendBuffer, 65, &bytesWritten, NULL);
#endif
	
	usbDevice->sendBufferWritePtr = usbDevice->sendBuffer;

end:
	if(lockSendBuffer)
		LeaveCriticalSection(&usbDevice->sendBufferMutex);
}

SKYETEK_STATUS 
USBDevice_Open(LPSKYETEK_DEVICE device)
{
	if( device == NULL )
		return SKYETEK_INVALID_PARAMETER;
  if( device->readFD != 0 && device->writeFD != 0 )
	  return SKYETEK_SUCCESS;

	device->readFD = CreateFile(device->address,
					GENERIC_READ,
					FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,
					NULL);

	if( device->readFD == INVALID_HANDLE_VALUE )
  {
    device->readFD = 0;
		return SKYETEK_READER_IO_ERROR;
  }

	device->writeFD = CreateFile(device->address,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,
					NULL);

	if( device->writeFD == INVALID_HANDLE_VALUE )
	{
		CloseHandle(device->readFD);
		device->readFD = device->writeFD = 0;
		return SKYETEK_READER_IO_ERROR;
	}

	return SKYETEK_SUCCESS;
}

SKYETEK_STATUS 
USBDevice_Close(LPSKYETEK_DEVICE device)
{
	if(device == NULL)
		return SKYETEK_INVALID_PARAMETER;
	
	if(device->readFD)
	{
		#ifndef WINCE
		CancelIo(device->writeFD);
		#endif
		CloseHandle(device->writeFD);
	}

	if(device->writeFD)
	{
		#ifndef WINCE
		CancelIo(device->readFD);
		#endif
		CloseHandle(device->readFD);
	}

	device->readFD = device->writeFD = 0;

	return SKYETEK_SUCCESS;
}

/* This function assumes we have locked the receiveBuffer mutex! */
unsigned int 
USBDevice_internalFillReceiveBuffer(LPSKYETEK_DEVICE device, unsigned int timeout)
{
	#ifndef WINCE
	OVERLAPPED overlap;
	#else
	COMMTIMEOUTS ctos;
	DEVICEIMPL* pImpl;
	#endif
	int bytesRead;
	unsigned char receiveBuffer[65];
	LPUSB_DEVICE usbDevice;

	if((device == NULL) || (device->user == NULL))
		return 0;

	usbDevice = (LPUSB_DEVICE)device->user;

	/* We emptied the buffer so reset all of our pointers */
	usbDevice->receiveBufferReadPtr = usbDevice->receiveBufferWritePtr = usbDevice->receiveBuffer;

	bytesRead = 0;

	#ifndef WINCE
		ZeroMemory(&overlap, sizeof(OVERLAPPED));
		overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		
		ReadFile(device->readFD, receiveBuffer, 65, &bytesRead, &overlap);

		if(WaitForSingleObject(overlap.hEvent, (timeout == 0) ? 100 : timeout) != WAIT_OBJECT_0)
		{
			CancelIo(device->readFD);
			CloseHandle(overlap.hEvent);
			return 0;
		}

		CloseHandle(overlap.hEvent);

		if(!HasOverlappedIoCompleted(&overlap))
		{
			CancelIo(device->readFD);
			return 0;
		}

		if(!GetOverlappedResult(device->readFD, &overlap, &bytesRead, 0))
			return 0;
	#else
		pImpl = (DEVICEIMPL*)device->internal;
		ZeroMemory(&ctos,sizeof(COMMTIMEOUTS));
		ctos.ReadTotalTimeoutConstant = pImpl->timeout;
		SetCommTimeouts(device->readFD, &ctos);
		ReadFile(device->readFD, receiveBuffer, 65, &bytesRead, NULL);
	#endif
	
	if(bytesRead != 65)
		return 0;

	CopyMemory(usbDevice->receiveBuffer, (receiveBuffer + 2), receiveBuffer[1]);
	usbDevice->receiveBufferWritePtr = usbDevice->receiveBuffer + receiveBuffer[1];

	return 1;
}
#endif

#ifdef HAVE_LIBUSB
void 
USBDevice_internalFlush(LPSKYETEK_DEVICE device, unsigned char lockSendBuffer)
{
	LPUSB_DEVICE usbDevice;
	unsigned char sendBuffer[64];
	int result;
	
	if((device == NULL) || (device->user == NULL))
		return;
	
	usbDevice = (LPUSB_DEVICE)device->user;

#ifdef HAVE_PTHREAD
	if(lockSendBuffer)
		pthread_mutex_lock(&usbDevice->sendBufferMutex);
#endif

	if(usbDevice->sendBufferWritePtr == usbDevice->sendBuffer)
		goto end;

	sendBuffer[0] = (usbDevice->sendBufferWritePtr - usbDevice->sendBuffer);
	memcpy((sendBuffer + 1), usbDevice->sendBuffer, sendBuffer[0]);

	/*printf("Writing - %d\r\n", sendBuffer[0]);
	for(size_t ix = 0; ix < sendBuffer[0]; ix++)
		printf("%02x", sendBuffer[ix]);
	printf("\r\n");*/

	
	if((result = usb_interrupt_write(usbDevice->usbDevHandle, 1, sendBuffer, 64, 100)) < 0)		
	{
		/*printf("usb_interrupt_write failed: %d \r\n", result);*/
	}
	else
		usbDevice->packetParity++;
	
	usbDevice->sendBufferWritePtr = usbDevice->sendBuffer;

end:
#ifdef HAVE_PTHREAD
	if(lockSendBuffer)
		pthread_mutex_unlock(&usbDevice->sendBufferMutex);
#endif
}

/* This function assumes we have locked the receiveBuffer mutex! */
unsigned int 
USBDevice_internalFillReceiveBuffer(LPSKYETEK_DEVICE device, unsigned int timeout)
{
	unsigned char receiveBuffer[64];
	LPUSB_DEVICE usbDevice;
	int result;
	unsigned char retried;
#ifdef LINUX
	unsigned char flushBuffer[3];
#endif

	if((device == NULL) || (device->user == NULL))
		return 0;

	usbDevice = (LPUSB_DEVICE)device->user;

	/* We emptied the buffer so reset all of our pointers */
	usbDevice->receiveBufferReadPtr = usbDevice->receiveBufferWritePtr = usbDevice->receiveBuffer;

#ifdef LINUX
	retried = 0;
again:
#endif
	if((result = usb_interrupt_read(usbDevice->usbDevHandle, 1, receiveBuffer, 64, (timeout == 0) ? 100 : timeout)) < 0)
	{
		/*printf("usb_interrupt_read failed: %d\r\n", result);*/
	
#ifdef LINUX
		if(!retried)
		{
			flushBuffer[0] = 0x00;
			flushBuffer[1] = 0x00;
			flushBuffer[2] = 0x00;
		
			USBDevice_Write(device, flushBuffer, 3, 100);
			USBDevice_Flush();
			retried = 1;
		}
#endif
		return 0;
	}

#ifdef LINUX
	usbDevice->packetParity++;
#endif

	/*printf("Reading - %d \r\n", receiveBuffer[0]);
	for(size_t ix = 0; ix < receiveBuffer[0]; ix++)
		printf("%02x", receiveBuffer[ix]);
	printf("\r\n");*/
	
	memcpy(usbDevice->receiveBuffer, (receiveBuffer + 1), receiveBuffer[0]);
	usbDevice->receiveBufferWritePtr = usbDevice->receiveBuffer + receiveBuffer[0];

	return 1;
}


SKYETEK_STATUS 
USBDevice_Close(LPSKYETEK_DEVICE device)
{
	LPUSB_DEVICE usbDevice;
	unsigned char flushBuffer[8];
	
	
	if((device == NULL) || (device->user == NULL))
		return SKYETEK_INVALID_PARAMETER;

	usbDevice = (LPUSB_DEVICE)device->user;
	
	/*printf("packetParity = %d\r\n", usbDevice->packetParity);*/

	/*if(usbDevice->packetParity % 2)
	{
		flushBuffer[0] = 0x00;
		flushBuffer[1] = 0x00;
		flushBuffer[2] = 0x00;
		
		USBDevice_Write(device, flushBuffer, 3, 100);
		USBDevice_Flush();
		//USBDevice_Read(device, flushBuffer, 7, 100);
		printf("Sent parityPacket\n");
	}*/
	
	#ifdef LINUX
	//usb_release_interface(usbDevice->usbDevHandle,0);
	//usb_reset(usbDevice->usbDevHandle);
	//usb_close(usbDevice->usbDevHandle);
	usb_reset(usbDevice->usbDevHandle);
	//printf("libusb: usb_reset\n");
	#endif 

	device->writeFD = device->readFD = 0;
	return SKYETEK_SUCCESS;
}

SKYETEK_STATUS
USBDevice_Open(LPSKYETEK_DEVICE device)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	LPUSB_DEVICE usbDevice;
	unsigned char retries;

	//added for debug
	char DriverName[32];

	
	if((device == NULL) || (device->user == NULL))
		return SKYETEK_INVALID_PARAMETER;
	
	if( device->readFD != 0 && device->writeFD != 0 )
	  return SKYETEK_SUCCESS;
	
	usbDevice = (LPUSB_DEVICE)device->user;

start:
	usb_busses = usb_get_busses ();
	for(bus = usb_busses; bus; bus = bus->next)
	{
		for(dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor == 0xafef)
			{
				//printf ("libusb: found idVendor= afef\n");
				usbDevice->usbDevHandle = usb_open(dev);
				//printf ("libusb: usb_open\n");

				if(usb_get_driver_np (usbDevice->usbDevHandle, 0, DriverName, 31) == 0)
				{
					//printf("libusb: driver = %s\n", DriverName);
					//printf("libusb: usb_get_driver_np\n");
					if(strcmp(DriverName, "usbhid") == 0)
					{
						//printf("libusb: device claimed by usbhid\n");
						if(usb_detach_kernel_driver_np(usbDevice->usbDevHandle, 0) == 0)
						{
							//printf("libusb: usb_detach_kernel_driver_np\n");
						}
						else
						{
							//printf("libusb: FAIL usb_detach_kernel_driver_np\n");
						}
						if(usb_set_configuration(usbDevice->usbDevHandle, 1) == 0)
						{
							//printf("libusb: usb_set_configuration\n");
						}
						else
						{
							//printf("libusb: FAIL usb_set_configuration\n");
						}
						if(usb_claim_interface(usbDevice->usbDevHandle, 0) == 0)
						{
							//printf("libusb: usb_claim_interface\n");
						}
						else
						{
							//printf("libusb: FAIL usb_claim_interface\n");
						}
						
					}
					else
					{
						//printf("libusb: device NOT claimed by usbhid\n");
					}

				}
				else
				{
					//printf("libusb: FAIL usb_get_driver_np\n");
				}

				/*
				retries = 3;
				while((usb_claim_interface(usbDevice->usbDevHandle, 0) != 0) && retries--)
				{
					//printf("Unable to claim interface\n");
					#ifdef LINUX
					if(usb_detach_kernel_driver_np(usbDevice->usbDevHandle, 0) < 0)
					{
						printf("Unable to detach kernel driver...\n");
						goto done;
					}
					#else
					goto done;
					#endif

				}
				
				usb_set_configuration(usbDevice->usbDevHandle, 1);

				#ifdef LINUX
				if(retries != 3)
				{
					printf("retries != 3\n");
					usb_close(usbDevice->usbDevHandle);
					usbDevice->usbDevHandle = NULL;
					goto start;
				}

				if(retries == 0)
					goto done;
				#endif
				*/

				/* Need this to make STPv3 happy */
				device->writeFD = device->readFD = 1;
				//#ifdef LINUX
				//usbDevice->packetParity = 0;
				//#endif
				
				return SKYETEK_SUCCESS;
			}
		}
	}

done:
	usb_close(usbDevice->usbDevHandle);
	usbDevice->usbDevHandle = NULL;
	return SKYETEK_READER_IO_ERROR;
}
#endif

#if defined(LINUX) || defined(WIN32)
int 
USBDevice_Write(LPSKYETEK_DEVICE device,
		unsigned char* buffer,
		unsigned int length,
    unsigned int timeout)
{
	unsigned char* ptr;
	unsigned char writeSize;
	LPUSB_DEVICE usbDevice;
	
	if((device == NULL) || (buffer == NULL) || (device->user == NULL) )
		return 0;

	usbDevice = (LPUSB_DEVICE)device->user;

	ptr = buffer;

	MUTEX_LOCK(&usbDevice->sendBufferMutex);
	while(length > 0)
	{
		writeSize = usbDevice->endOfSendBuffer - (unsigned int)usbDevice->sendBufferWritePtr;

		writeSize = (length > writeSize) ? writeSize : length;

		memcpy(usbDevice->sendBufferWritePtr, ptr, writeSize);
		
		usbDevice->sendBufferWritePtr += writeSize;
		ptr += writeSize;
		length -= writeSize;

		if((unsigned int)usbDevice->sendBufferWritePtr == usbDevice->endOfSendBuffer)
			USBDevice_internalFlush(device, 0);
	}
	MUTEX_UNLOCK(&usbDevice->sendBufferMutex);

	return (ptr - buffer);
}

int 
USBDevice_Read(LPSKYETEK_DEVICE device,
		unsigned char* buffer,
		unsigned int length,
    unsigned int timeout
    )
{
	
	unsigned char readSize;
	unsigned char* ptr;
	LPUSB_DEVICE usbDevice;
  LPDEVICEIMPL di;

	if( (device == NULL) || (buffer == NULL) || (device->user == NULL) || (device->internal == NULL))
		return 0;

  di = (LPDEVICEIMPL)device->internal;

	usbDevice = (LPUSB_DEVICE)device->user;
	
	USBDevice_internalFlush(device, 0);
	
	ptr = buffer;

	MUTEX_LOCK(&usbDevice->receiveBufferMutex);
	while(1)
	{
		/* Check to see if we have data buffered */
		if((usbDevice->receiveBufferReadPtr != usbDevice->receiveBufferWritePtr)
			&& (usbDevice->receiveBufferWritePtr != usbDevice->receiveBuffer))
		{
			readSize = usbDevice->receiveBufferWritePtr - usbDevice->receiveBufferReadPtr; 
			readSize = (length > readSize) ? readSize : length;
			memcpy(ptr, usbDevice->receiveBufferReadPtr, readSize);
			usbDevice->receiveBufferReadPtr += readSize;
			length -= readSize;
			ptr += readSize;
		}
		
		if(length == 0)
			goto end;
		
		if(!USBDevice_internalFillReceiveBuffer(device, timeout + di->timeout))
			goto end;
	};

end:
	MUTEX_UNLOCK(&usbDevice->receiveBufferMutex);
	return (ptr - buffer);
}

void 
USBDevice_Flush(LPSKYETEK_DEVICE device)
{
	if(device == NULL)
		return;

	USBDevice_internalFlush(device, 1);
}

int 
USBDevice_Free(LPSKYETEK_DEVICE device)
{
	LPUSB_DEVICE usbDevice;

	if(device == NULL)
		return 0;

	if(device->user == NULL)
		return 0;

	USBDevice_Close(device);

	usbDevice = (LPUSB_DEVICE)device->user;

	MUTEX_DESTROY(&usbDevice->receiveBufferMutex);
	MUTEX_DESTROY(&usbDevice->sendBufferMutex);

	free(usbDevice);
	device->user = NULL;

	free(device);
	return 1;
}

SKYETEK_STATUS
USBDevice_SetAdditionalTimeout(
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
USBDevice_InitDevice(LPSKYETEK_DEVICE device)
{
	LPUSB_DEVICE usbDevice;

	if( device == NULL )
		return;

	device->internal = &USBDeviceImpl;
	
	usbDevice = (LPUSB_DEVICE)malloc(sizeof(USB_DEVICE));
	memset(usbDevice, 0, sizeof(USB_DEVICE));

	usbDevice->sendBufferWritePtr = usbDevice->sendBuffer;
	usbDevice->endOfSendBuffer = (unsigned int)usbDevice->sendBuffer
					+ (63 * sizeof(unsigned char));
							

	usbDevice->receiveBufferReadPtr 
		= usbDevice->receiveBufferWritePtr = usbDevice->receiveBuffer;
	usbDevice->endOfReceiveBuffer = (unsigned int)usbDevice->receiveBuffer
					+ (63 * sizeof(unsigned char));
	
	MUTEX_CREATE(&usbDevice->sendBufferMutex);
	MUTEX_CREATE(&usbDevice->receiveBufferMutex);
	
	usbDevice->packetParity = 0;
	
	device->user = (void*)usbDevice;
}

DEVICEIMPL USBDeviceImpl = {
  USBDevice_Open,
	USBDevice_Close,
	USBDevice_Read,
	USBDevice_Write,
	USBDevice_Flush,
	USBDevice_Free,
  USBDevice_SetAdditionalTimeout,
  0
};
#endif
