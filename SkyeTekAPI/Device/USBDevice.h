/**
 * USBDevice.h
 * Copyright � 2006 - 2008 Skyetek, Inc. All Rights Reserved.
 *
 */
#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include "../SkyeTekAPI.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* This structure is used internally by the USBDevice driver */
typedef struct USB_DEVICE {
	unsigned int packetParity;
	unsigned char* sendBufferWritePtr;
	unsigned int endOfSendBuffer;
	unsigned char* receiveBufferWritePtr;
	unsigned char* receiveBufferReadPtr;
	unsigned int endOfReceiveBuffer;
	MUTEX(sendBufferMutex);
	MUTEX(receiveBufferMutex);
#ifdef HAVE_LIBUSB
	struct usb_dev_handle* usbDevHandle;
#endif
	unsigned char sendBuffer[63];
	unsigned char receiveBuffer[63];
} USB_DEVICE, *LPUSB_DEVICE;

/**
 * Initializes the newly created USB device by 
 * setting up its function table.
 * @param device The device
 */
void USBDevice_InitDevice(
  LPSKYETEK_DEVICE device
  );

/**
 * Frees any resources used internally by the device
 * @param device The device to destroy
 */
int 
USBDevice_Free(
  LPSKYETEK_DEVICE    device
  );

void
USBDevice_Flush(LPSKYETEK_DEVICE device);


#ifdef __cplusplus
}
#endif


#endif
