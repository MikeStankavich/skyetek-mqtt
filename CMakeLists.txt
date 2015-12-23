cmake_minimum_required(VERSION 3.0)
project(skyetek_mqtt)

#find_path(USB_INCLUDE_DIR libusb.h /usr/include/usb)
#find_path(MALLOC_INCLUDE_DIR malloc.h /usr/include/sys)
find_path(PAHO_INCLUDE_DIR NAMES MQTTClient.h)

find_library(PAHO_LIBRARY NAMES libpaho-mqtt3c.so)
find_library(LIBUSB_LIBRARY NAMES usb)

include_directories(SkyeTekAPI)

add_definitions( -DLINUX -DHAVE_PTHREAD -DHAVE_LIBUSB )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
set(LIBRARY_FILES
        SkyeTekAPI/SkyeTekAPI.c
        SkyeTekAPI/Device/DeviceFactory.c
        SkyeTekAPI/Device/SerialDevice.c
        SkyeTekAPI/Device/SerialDeviceFactory.c
#        SkyeTekAPI/Device/SPIDevice.c
#        SkyeTekAPI/Device/SPIDeviceFactory.c
        SkyeTekAPI/Device/USBDevice.c
        SkyeTekAPI/Device/USBDeviceFactory.c
        SkyeTekAPI/Drivers/aardvark.c
        SkyeTekAPI/Protocol/asn1.c
        SkyeTekAPI/Protocol/CRC.c
        SkyeTekAPI/Protocol/STPv2.c
        SkyeTekAPI/Protocol/STPv3.c
        SkyeTekAPI/Protocol/utils.c
        SkyeTekAPI/Reader/ReaderFactory.c
        SkyeTekAPI/Reader/SkyeTekReader.c
        SkyeTekAPI/Reader/SkyeTekReaderFactory.c
        SkyeTekAPI/Tag/DesfireTag.c
        SkyeTekAPI/Tag/GenericTag.c
        SkyeTekAPI/Tag/Iso14443ATag.c
        SkyeTekAPI/Tag/Iso14443BTag.c
        SkyeTekAPI/Tag/Tag.c
        SkyeTekAPI/Tag/TagFactory.c)

ADD_LIBRARY(SkyeTekAPI STATIC ${LIBRARY_FILES})

add_executable(skyetek_mqtt ${SOURCE_FILES})
target_link_libraries(skyetek_mqtt ${PAHO_LIBRARY} SkyeTekAPI ${LIBUSB_LIBRARY} )


