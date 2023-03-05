/*
 * Copyright 2020 Tyler Qiu.
 * YUV420 to RGBA open source project.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.cyl.camera2libyuv.utils;


/**
 * @author Yulin Cao
 * @date: 2023/03/02
 */
public class LibyuvUtils {
    static {
        System.loadLibrary("YuvUtils");
    }

    public static native void NV12Scale(byte[] src_y,
                                        int src_stride_y,
                                        byte[] src_uv,
                                        int src_stride_uv,
                                        int src_w, int src_h,
                                        byte[] dst_y,
                                        int dst_stride_y,
                                        byte[] dst_uv,
                                        int dst_stride_uv,
                                        int dst_w, int dst_h);

    public static native void NV21Scale(byte[] src_y,
                                        int src_stride_y,
                                        byte[] src_vu,
                                        int src_stride_vu,
                                        int src_w, int src_h,
                                        byte[] dst_y,
                                        int dst_stride_y,
                                        byte[] dst_vu,
                                        int dst_stride_vu,
                                        int dst_w, int dst_h);

}
