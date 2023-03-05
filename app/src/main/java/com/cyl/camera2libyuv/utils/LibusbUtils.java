package com.cyl.camera2libyuv.utils;

public class LibusbUtils {

    static {
        System.loadLibrary("UsbUtils");
    }


    public static native void init(String path);

    public static native void sendData(byte[] datas, int size);
}
