cmake_minimum_required(VERSION 3.10.2)
project("swordfish")

if(ANDROID_ABI STREQUAL "armeabi-v7a")
  include_directories(${ANDROID_SYSROOT}/usr/include/arm-linux-androideabi)
elseif(ANDROID_ABI STREQUAL "arm64-v8a")
  include_directories(${ANDROID_SYSROOT}/usr/include/aarch64-linux-android)
else()
  include_directories(${ANDROID_SYSROOT}/usr/include/arm-linux-androideabi)
endif()
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "-pthread -DANDROID -O3 -std=c++11 -Wall -Wextra ")# -DDEBUGLOG")
#set(CMAKE_C_FLAGS "-O3 -Wall -Wextra -DDEBUGLOG")
#set(CMAKE_CXX_FLAGS "-O3 -std=c++11 -Wall -Wextra")
set(CMAKE_C_FLAGS "-pthread -O3 -Wall -Wextra ")
#set(CMAKE_C_FLAGS "-pthread -O3 -Wall -Wextra -DUSEDLOPENLIBUSB")

include_directories(libusb-1.0.24/libusb include)

include_directories(./ ./libusb-1.0.24)

add_subdirectory(libusb-1.0.24)

add_library(swordfish SHARED
  src/yuv_rgb.c
  src/swordfish.c
  src/utils.c
  src/log.c
  src/log_file.c
  src/list.c
  src/trace.c
  src/timer.c
  src/ring_queue.c
  )
target_link_libraries(swordfish usb)

#file(GLOB USBLIB libusb1.0.so)
#
#add_executable(test_swordfish test_swordfish.cpp)
#
#target_link_libraries(test_swordfish swordfish ${USBLIB})
