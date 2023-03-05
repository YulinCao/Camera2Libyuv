#include <jni.h>
#include <string>
#include "libyuv_utils.h"
#include "libyuv/scale.h"

extern "C"
JNIEXPORT void JNICALL
Java_com_cyl_camera2libyuv_utils_LibyuvUtils_NV12Scale(JNIEnv *env,
                                          jclass clazz,
                                          jbyteArray src_y,
                                          jint src_stride_y,
                                          jbyteArray src_uv,
                                          jint src_stride_uv,
                                          jint width, jint height,
                                          jbyteArray dst_y,
                                          jint dst_stride_y,
                                          jbyteArray dst_uv,
                                          jint dst_stride_uv,
                                          jint dst_width,
                                          jint dst_height
) {
    jbyte *y_src = env->GetByteArrayElements(src_y, nullptr);
    jbyte *y_dst = env->GetByteArrayElements(dst_y, nullptr);

    jbyte *uv_src = env->GetByteArrayElements(src_uv, nullptr);
    jbyte *uv_dst = env->GetByteArrayElements(dst_uv, nullptr);

    libyuv::NV12Scale( (uint8_t *)y_src,
                       src_stride_y,
                       (uint8_t *)uv_src,
                       src_stride_uv,
                       width,
                       height,
                       (uint8_t *)y_dst,
                       dst_stride_y,
                       (uint8_t *)uv_dst,
                       dst_stride_uv,
                       dst_width,
                       dst_height,
                       libyuv::kFilterBilinear);

    env->ReleaseByteArrayElements(src_y, y_src, JNI_ABORT);
    env->ReleaseByteArrayElements(src_uv, uv_src, JNI_ABORT);

    env->ReleaseByteArrayElements(dst_y, y_dst, 0);
    env->ReleaseByteArrayElements(dst_uv, uv_dst, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_cyl_camera2libyuv_utils_LibyuvUtils_NV21Scale(JNIEnv *env,
                                          jclass clazz,
                                          jbyteArray src_y,
                                          jint src_stride_y,
                                          jbyteArray src_vu,
                                          jint src_stride_vu,
                                          jint width, jint height,
                                          jbyteArray dst_y,
                                          jint dst_stride_y,
                                          jbyteArray dst_vu,
                                          jint dst_stride_vu,
                                          jint dst_width,
                                          jint dst_height
) {
    jbyte *y_src = env->GetByteArrayElements(src_y, nullptr);
    jbyte *y_dst = env->GetByteArrayElements(dst_y, nullptr);

    jbyte *vu_src = env->GetByteArrayElements(src_vu, nullptr);
    jbyte *vu_dst = env->GetByteArrayElements(dst_vu, nullptr);

    libyuv::NV12Scale( (uint8_t *)y_src,
                       src_stride_y,
                       (uint8_t *)vu_src,
                       src_stride_vu,
                       width,
                       height,
                       (uint8_t *)y_dst,
                       dst_stride_y,
                       (uint8_t *)vu_dst,
                       dst_stride_vu,
                       dst_width,
                       dst_height,
                       libyuv::kFilterBilinear);

    env->ReleaseByteArrayElements(src_y, y_src, JNI_ABORT);
    env->ReleaseByteArrayElements(src_vu, vu_src, JNI_ABORT);

    env->ReleaseByteArrayElements(dst_y, y_dst, 0);
    env->ReleaseByteArrayElements(dst_vu, vu_dst, 0);
}




