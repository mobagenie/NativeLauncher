#!/data/data/com.termux/files/usr/bin/bash

# Path root project
ROOT="app/src/main"

# Cari semua file .xml, .java, .cpp, .txt
find "$ROOT" \( -name "*.xml" -o -name "*.java" -o -name "*.cpp" -o -name "*.txt" \) | while read f; do
    echo "===== $f ====="
    cat "$f"
    echo
done | termux-clipboard-set