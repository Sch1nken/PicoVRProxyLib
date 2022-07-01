# compile_medium_example.sh

export NDK="/opt/android-ndk"
export TOOLCHAIN="${NDK}/toolchains/llvm/prebuilt/linux-x86_64"
export SYSROOT="${TOOLCHAIN}/sysroot"
export CPATH="./Headers"
export LIBRARY_PATH=$LIBRARY_PATH:./lib

export API=21 # set this to your minSdkVersion

# create array of clang binary names
declare -a ndk_clang_binaries=("armv7a-linux-androideabi" "aarch64-linux-android")

# create array of Android JNI folder names for library detection
declare -a jni_folder_names=("armeabi-v7a" "arm64-v8a")

for (( i=0; i<2; i++ ));
do
    TARGET_CLANG=${ndk_clang_binaries[$i]}
    OUTPUT_FOLDER="out/${jni_folder_names[$i]}"
    mkdir -p $OUTPUT_FOLDER
    $TOOLCHAIN/bin/$TARGET_CLANG$API-clang --sysroot=$SYSROOT -fPIC -shared -o $OUTPUT_FOLDER/picoAPI.so lib/${jni_folder_names[$i]}/libpxr_api.so picoAPI.c
done
