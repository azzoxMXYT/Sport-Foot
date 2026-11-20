#!/usr/bin/env bash
# Run from the repository root, with Android SDK and NDK installed on Linux.
set -eu
: "${ANDROID_NDK_HOME:?NDK r27+}" "${ANDROID_SDK_ROOT:?Android SDK}" "${BUILD_TOOLS:?path to SDK build-tools, e.g. build-tools/35.0.0 (absolute path)}"
out=build/android
mkdir -p "$out/res/drawable-nodpi" "$out/lib/arm64-v8a"
python3 tools/art.py
cp art/icon.png "$out/res/drawable-nodpi/icon.png"
"$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang" \
  -std=c11 -O2 -fPIC -shared -Wl,-z,max-page-size=16384 -Wl,--no-undefined \
  src/game.c src/android.c -landroid -o "$out/lib/arm64-v8a/libsportfoot.so"
"$BUILD_TOOLS/aapt2" compile --dir "$out/res" -o "$out/resources.zip"
"$BUILD_TOOLS/aapt2" link -I "$ANDROID_SDK_ROOT/platforms/android-35/android.jar" \
  --manifest android/AndroidManifest.xml "$out/resources.zip" -o "$out/game.apk"
(cd "$out" && zip -q game.apk lib/arm64-v8a/libsportfoot.so)
"$BUILD_TOOLS/zipalign" -f 4 "$out/game.apk" "$out/unsigned.apk"
echo "APK à signer : $out/unsigned.apk"
