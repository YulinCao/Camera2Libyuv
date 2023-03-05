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

#ifndef LIBYUV_UTILS_H
#define LIBYUV_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

int libyuvNV12Scale(unsigned char *src_y,
                    int src_stride_y,
                    unsigned char *src_uv,
                    int src_stride_uv,
                    int src_w, int src_h,
                    unsigned char *dst_y,
                    int dst_stride_y,
                    unsigned char *dst_uv,
                    int dst_stride_uv,
                    int dst_w, int dst_h);

int libyuvNV21Scale(unsigned char *src_y,
                    int src_stride_y,
                    unsigned char *src_uv,
                    int src_stride_uv,
                    int src_w, int src_h,
                    unsigned char *dst_y,
                    int dst_stride_y,
                    unsigned char *dst_uv,
                    int dst_stride_uv,
                    int dst_w, int dst_h);

#ifdef __cplusplus
}
#endif
#endif //LIBYUV_UTILS_H
