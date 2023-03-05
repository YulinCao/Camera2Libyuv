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
#include <stdint.h>
#include <libyuv/convert.h>
#include <libyuv/convert_argb.h>
#include <libyuv/convert_from.h>
#include <libyuv/rotate.h>
#include <libyuv/rotate_argb.h>
#include <cstdlib>

#include "libyuv_utils.h"
#include "libyuv/scale.h"

using namespace std;
using namespace libyuv;

int libyuvNV12Scale(unsigned char *src_y,
                    int src_stride_y,
                    unsigned char *src_uv,
                    int src_stride_uv,
                    int src_w, int src_h,
                    unsigned char *dst_y,
                    int dst_stride_y,
                    unsigned char *dst_uv,
                    int dst_stride_uv,
                    int dst_w, int dst_h){

    libyuv::NV12Scale( (uint8_t *)src_y,
                      src_stride_y,
                       (uint8_t *)src_uv,
                      src_stride_uv,
                      src_w,
                      src_h,
                       (uint8_t *)dst_y,
                      dst_stride_y,
                       (uint8_t *)dst_uv,
                      dst_stride_uv,
                      dst_w,
                      dst_h,
                      libyuv::kFilterBilinear);
}

int libyuvNV21Scale(unsigned char *src_y,
                    int src_stride_y,
                    unsigned char *src_uv,
                    int src_stride_uv,
                    int src_w, int src_h,
                    unsigned char *dst_y,
                    int dst_stride_y,
                    unsigned char *dst_uv,
                    int dst_stride_uv,
                    int dst_w, int dst_h){
}