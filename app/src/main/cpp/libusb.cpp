//
// Created by changjun.cao on 2023/3/3.
//
#include <jni.h>
#include <string>
#include "libusb_utils.h"

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
//    usbDeviceDelInit();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_cyl_camera2libyuv_utils_LibusbUtils_init(JNIEnv *env,jclass clazz, jstring path) {
    const char* str = env->GetStringUTFChars(path,0);
    usbDeviceInit(str);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_cyl_camera2libyuv_utils_LibusbUtils_sendData(JNIEnv *env,
                                                       jclass clazz,
                                                       jbyteArray datas,
                                                       jint size) {

    jbyte *_datas = env->GetByteArrayElements(datas, nullptr);
    usbDeviceDataSent((uint8_t *)_datas, size);

    env->ReleaseByteArrayElements(datas, _datas, JNI_ABORT);
}



