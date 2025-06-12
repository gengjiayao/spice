#!/usr/bin/env bash

set -euo pipefail

TARGET_DIR="/home/qingyangwang/Linux_Gutai2/jiayao/strumpack-8.0.0/build"

# 确保目标目录存在
if [ ! -d "$TARGET_DIR" ]; then
  echo "Error: 目录 '$TARGET_DIR' 不存在。"
  exit 1
fi

# 获取脚本所在目录（与当前工作目录无关）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo ">> 切换到目标目录：$TARGET_DIR"
pushd "$TARGET_DIR" > /dev/null

echo ">> 在 $TARGET_DIR 下执行 make"
make

echo ">> 在 $TARGET_DIR 下执行 make install"
make install

echo ">> 返回脚本目录：$SCRIPT_DIR"
popd > /dev/null

echo ">> 切换到脚本目录：$SCRIPT_DIR"
cd "$SCRIPT_DIR"
cd build

echo ">> 在脚本目录下执行 make"
make

echo ">> 全部操作完成！"
