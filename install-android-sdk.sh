#!/bin/bash

set -e

SDK_DIR="$HOME/android-sdk"
CMDLINE_URL="https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip"

echo "🔧 Fixing possible Yarn GPG issue (if exists)..."
if [ -f /etc/apt/sources.list.d/yarn.list ]; then
  sudo mkdir -p /etc/apt/keyrings
  curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | sudo gpg --dearmor -o /etc/apt/keyrings/yarn.gpg || true
  echo "deb [signed-by=/etc/apt/keyrings/yarn.gpg] https://dl.yarnpkg.com/debian stable main" | sudo tee /etc/apt/sources.list.d/yarn.list > /dev/null || true
fi

echo "📦 Install dependency..."
sudo apt update -y
sudo apt install -y unzip wget openjdk-17-jdk

echo "📁 Membuat folder SDK..."
mkdir -p $SDK_DIR/cmdline-tools
cd $SDK_DIR

echo "⬇️ Download Android command line tools..."
wget -q $CMDLINE_URL -O cmdline-tools.zip

echo "📂 Extract..."
unzip -q cmdline-tools.zip -d cmdline-tools
mv cmdline-tools/cmdline-tools cmdline-tools/latest
rm cmdline-tools.zip

echo "🌎 Set environment..."
export ANDROID_HOME=$SDK_DIR
export PATH=$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$PATH

grep -q ANDROID_HOME ~/.bashrc || echo "export ANDROID_HOME=$SDK_DIR" >> ~/.bashrc
grep -q cmdline-tools ~/.bashrc || echo 'export PATH=$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$PATH' >> ~/.bashrc

echo "📦 Install platform & build tools..."
yes | sdkmanager --licenses > /dev/null
sdkmanager "platform-tools" "platforms;android-34" "build-tools;34.0.0"

echo "📝 Buat local.properties..."
cd /workspaces/NativeLauncher
echo "sdk.dir=$SDK_DIR" > local.properties

echo ""
echo "✅ ANDROID SDK BERHASIL DIINSTALL!"
echo "Sekarang jalankan:"
echo "   source ~/.bashrc"
echo "   gradle wrapper"
