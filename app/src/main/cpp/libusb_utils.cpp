//
// Created by changjun.cao on 2023/3/3.
//

#include "libusb_utils.h"
#include "swordfish.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <jni.h>
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Yolo-debug", __VA_ARGS__)


static SWORDFISH_DEVICE_HANDLE handle = NULL;

static long count = 0;
void resultcallback(void *data, void *userdata)
{
    if (userdata != NULL)
    {
        // printf("userdata:0x%lX\n", (long)userdata);
    }

    SWORDFISH_USBHeaderDataPacket *tmppack = (SWORDFISH_USBHeaderDataPacket *)data;
    printf("recv pkt,type:%d,subtype:%d,datalen:%d\n",
           tmppack->type, tmppack->sub_type, tmppack->len);

    if(tmppack ){
        uint8_t *data = tmppack->data;
//        std::string s((char *) data);

        std::string code_str;

        for (int i = 0; i < 100;i++)
        {
            //分离16进制数的"十位"和“个位”
            char s1 = char(data[i] >> 4);
            char s2 = char(data[i] & 0xf);
            //将分离得到的数字转换成对应的ASCII码，数字和字母分开，统一按照小写处理
            s1 > 9 ? s1 += 87 : s1 += 48;
            s2 > 9 ? s2 += 87 : s2 += 48;
            //将处理好的字符放入到string中
            code_str.append(1,s1);
            code_str.append(1, s2);
        }

        LOGD("Camera2 npu responce result %s  %i", code_str.c_str(), tmppack->len);

        std::string name;
        name.append("/storage/emulated/0/");
        name.append(std::to_string(count));
        name.append(".bin");
        FILE *dumpFile = fopen(name.c_str(), "w+");

        fwrite((char *) data, sizeof(char), 2400, dumpFile);
        fclose(dumpFile);

        count++;
    }
}

int usbDeviceInit(const char* str){

//    swordfish_init ("swordfish.log", 1024 * 1024);

    swordfish_init (str, 1024 * 1024);
    int total = 0;
    SWORDFISH_DEVICE_INFO *tmpdevice = NULL;
    if (SWORDFISH_OK == swordfish_enum_device (&total, &tmpdevice)) {
        printf ("found swordfish device,sum:%d,first usb_camera_name:%s\n", total,
                tmpdevice[0].usb_camera_name);
        handle = swordfish_open (tmpdevice);
    } else {
        printf ("there's no swordfish usb camera plugged!\n");
    }

    if (NULL == handle) {
        printf ("fail to connect swordfish device\n");
//        exit (0);
    }

    swordfish_setcallback(handle,resultcallback,NULL);

    return 0;

}

int usbDeviceDataSent(uint8_t *buffer, int size){

    if (swordfish_sendbuffer (handle, buffer, size,
                              1024 * 1024) == SWORDFISH_OK) {
        // Todo success

    } else {
        // Todo fail
    }

    return 0;
}


int usbDeviceDelInit(){
    swordfish_deinit ();
    return 0;
}

