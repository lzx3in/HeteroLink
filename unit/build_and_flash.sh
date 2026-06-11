#!/bin/bash
# 设置 ESP-IDF 环境
export IDF_PATH=~/esp/esp-idf
export PATH=$IDF_PATH/tools:$PATH
source $IDF_PATH/export.sh

echo "=== Building firmware ==="
idf.py build

if [ $? -eq 0 ]; then
    echo "=== Build successful ==="
    echo "Ready to flash. Run: idf.py -p /dev/ttyUSB0 flash monitor"
else
    echo "=== Build failed ==="
    exit 1
fi
