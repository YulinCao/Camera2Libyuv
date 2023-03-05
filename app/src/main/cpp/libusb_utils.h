//
// Created by changjun.cao on 2023/3/3.
//

#ifndef CAMERA2LIBYUV_LIBUSB_UTILS_H
#define CAMERA2LIBYUV_LIBUSB_UTILS_H

#include "stdlib.h"
#ifdef __cplusplus
extern "C" {
#endif

int usbDeviceInit(const char* str);

void resultcallback(void *data, void *userdata);

int usbDeviceDelInit();

int usbDeviceDataSent(uint8_t *buffer, int size);

#ifdef __cplusplus
}
#endif

#endif //CAMERA2LIBYUV_LIBUSB_UTILS_H
