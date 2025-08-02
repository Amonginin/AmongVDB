#!/bin/bash

echo "======================"
echo "开始编译流程"
echo "======================"

# 一、安装BLAS前添加blas.sh可执行权限
BLAS_SCRIPT="../../env/blas.sh"
echo "检查并添加 $BLAS_SCRIPT 执行权限..."
if [ ! -x "$BLAS_SCRIPT" ]; then
    chmod +x "$BLAS_SCRIPT"
    echo "已为 $BLAS_SCRIPT 添加可执行权限"
else
    echo "$BLAS_SCRIPT 已有执行权限"
fi

# 执行blas.sh安装blas库
echo "开始安装 BLAS 库..."
if "$BLAS_SCRIPT"; then
    echo "BLAS 库安装成功"
else
    echo "BLAS 库安装失败，退出脚本"
    exit 1
fi

# 二、执行make时动态获取系统线程数
echo "获取系统CPU核心数..."
THREADS=$(nproc --all)  # 获取全部逻辑核心数
# THREADS=$(nproc --physical)  # 如需物理核心数，取消注释此行

echo "执行 make clean..."
make clean
echo "执行 make -j$THREADS..."  # 使用动态线程数
make -j"$THREADS" || make

echo "======================"
echo "开始启动流程"
echo "======================"

# 解决启动编译好的vdb_server时无法加载librocksdb.so的问题
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/../rocksdb
echo "已设置 LD_LIBRARY_PATH: $LD_LIBRARY_PATH"

# 启动vdb_server
echo "启动 vdb_server..."
if ./build/vdb_server; then
    echo "vdb_server 启动成功"
else
    echo "vdb_server 启动失败"
    exit 1
fi