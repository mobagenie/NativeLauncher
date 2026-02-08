#!/bin/bash

echo "🔍 Mencari Android SDK..."

# Kandidat lokasi SDK
CANDIDATES=(
"$HOME/android-sdk"
"$HOME/Android/Sdk"
"/usr/local/android-sdk"
)

SDK_PATH=""

for path in "${CANDIDATES[@]}"; do
  if [ -d "$path" ]; then
    SDK_PATH="$path"
    break
  fi
done

if [ -z "$SDK_PATH" ]; then
  echo "❌ Android SDK tidak ditemukan."
  echo "Silakan install dulu Android SDK."
  exit 1
fi

echo "✅ SDK ditemukan di: $SDK_PATH"

# Export environment
export ANDROID_HOME="$SDK_PATH"
export PATH="$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$PATH"

# Simpan permanen
grep -q ANDROID_HOME ~/.bashrc || echo "export ANDROID_HOME=$SDK_PATH" >> ~/.bashrc
grep -q cmdline-tools ~/.bashrc || echo 'export PATH=$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$PATH' >> ~/.bashrc

# Buat local.properties
echo "sdk.dir=$SDK_PATH" > local.properties

echo "✅ local.properties dibuat"
echo "✅ ANDROID_HOME diset"
echo "✅ PATH diset"

echo "🔎 Testing..."
which sdkmanager || echo "⚠️ sdkmanager tidak ditemukan"
which adb || echo "⚠️ adb tidak ditemukan"

echo ""
echo "🎉 Setup selesai! Sekarang coba:"
echo "   gradle wrapper"
