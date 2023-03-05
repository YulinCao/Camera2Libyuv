install ndk:
1.download ndk r18b from https://dl.google.com/android/repository/android-ndk-r18b-linux-x86_64.zip
2.unzip android-ndk-r18b-linux-x86_64.zip to /home/$USER/workspace/android-ndk-r18b
3.add below to ~/.bashrc
```````````````
export NDK_ROOT=/home/$USER/workspace/android-ndk-r18b
export PATH=/opt/android/arm64/bin:$PATH
``````````````
4.install standalone toolchain
sudo mkdir /opt/android
sudo build/tools/make-standalone-toolchain.sh --platform=android-23 --install-dir=/opt/android/arm64 --use-llvm --arch=arm64

download liehu-feynman and build
1.cd ~/workspace && git clone https://gitee.com/nextvpu/sdk-feynman-camera -b swordfish swordfish
2.cd ~/workspace/swordfish
3.mkdir build && cd build
4.build source:
. ~/.bashrc
cmake .. -DANDROID_ABI=armeabi-v7a -DCMAKE_TOOLCHAIN_FILE=${NDK_ROOT}/build/cmake/android.toolchain.cmake -DANDROID_PLATFORM=android-21 -DCMAKE_C_COMPILER=arm-linux-androideabi-clang -DCMAKE_CXX_COMPILER=arm-linux-androideabi-clang++ -DANDROID_SYSROOT=/opt/android/armv7/sysroot
make


Notes:
How to build libusb1.0.24
1. cd ${LIBUSB_ROOT}/android/jni
2. ${NDK_ROOT}/ndk-build
3. cp ${LIBUSB_ROOT}/android/libs/arm64-v8a/libusb1.0.so ${LIBSWORDFISH}/libusb-1.0.24/android/libs/arm64-v8a/
