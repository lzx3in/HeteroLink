#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# HeteroLink - ESP-IDF 安装脚本
# 使用 EIM (ESP-IDF Installation Manager) 安装 ESP-IDF

set -e

echo "========================================"
echo "  HeteroLink - ESP-IDF 安装脚本"
echo "========================================"

# 检查是否已安装 EIM
if ! command -v eim &> /dev/null; then
    echo "正在安装 EIM..."
    pip install eim
fi

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CONFIG_FILE="$SCRIPT_DIR/eim_config.toml"

# 检查配置文件
if [ ! -f "$CONFIG_FILE" ]; then
    echo "错误：未找到配置文件 $CONFIG_FILE"
    exit 1
fi

echo "使用配置文件：$CONFIG_FILE"

# 安装 ESP-IDF
echo "正在安装 ESP-IDF..."
eim install --config "$CONFIG_FILE"

echo ""
echo "========================================"
echo "  安装完成！"
echo "========================================"
echo ""
echo "请运行以下命令激活 ESP-IDF 环境："
echo ""
echo "  . \$HOME/esp/esp-idf/export.sh"
echo ""
echo "或者将导出脚本添加到 ~/.bashrc："
echo ""
echo "  echo 'alias esp_idf_export=\". \$HOME/esp/esp-idf/export.sh\"' >> ~/.bashrc"
echo ""
