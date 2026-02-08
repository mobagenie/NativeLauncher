#!/bin/bash
./gradlew clean assembleDebug
#mkdir -p release
#cp app/build/outputs/apk/debug/app-debug.apk release/NativeLauncher.apk
echo "APK ready at release/NativeLauncher.apk"
