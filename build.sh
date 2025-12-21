#!/bin/bash

set -e

PROJECT_DIR=$(pwd)
BUILD_DIR=${PROJECT_DIR}/build
## 安装目录
INSTALL_PREFIX=/usr/local

echo "==> Build directory: ${BUILD_DIR}"
echo "==> Install prefix : ${INSTALL_PREFIX}"

# 如果没有build目录 创建build目录
if [ ! -d "${BUILD_DIR}" ]; then
    mkdir -p "${BUILD_DIR}"
fi

# 安全删除build里面的内容 防止误删
rm -rf "${BUILD_DIR:?}/"*

# 编译
cd "${BUILD_DIR}"
cmake ..
make -j$(nproc)

# 回到项目根目录下
cd ..

# 把头文件拷贝到 /usr/local/include/muduomy  so库拷贝到/usr/local/lib  PATH
INCLUDE_DST=${INSTALL_PREFIX}/include/muduomy
if [ ! -d "${INCLUDE_DST}" ]; then
    mkdir -p "${INCLUDE_DST}"
fi

# 将根项目中的头文件拷贝到安装目录
for header in `ls *.h`
do
    cp $header "${INCLUDE_DST}"
done

# 安装动态库
LIB_DST=${INSTALL_PREFIX}/lib
cp "${PROJECT_DIR}/lib/libmuduomy.so" "${LIB_DST}"

# 刷新 
ldconfig

echo "==> muduomy build and install sucessful!"