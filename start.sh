#!/bin/bash

echo "======================"
echo "开始编译和启动流程"
echo "======================"

# 执行blas.sh安装blas库（需sudo权限）
echo "开始安装 BLAS 库..."
if ../../env/blas.sh; then
    echo "BLAS 库安装成功"
else
    echo "BLAS 库安装失败，退出脚本"
    exit 1
fi

# 清理并编译
echo "执行 make clean..."
make clean
echo "执行 make -j8..."
make -j8

# 解决启动编译好的vdb_server时无法加载librocksdb.so的问题
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(dirname $0)/../rocksdb
echo "已设置 LD_LIBRARY_PATH: $LD_LIBRARY_PATH"

# 启动vdb_server
echo "启动 vdb_server..."
if ./build/vdb_server; then
    echo "vdb_server 启动成功"
else
    echo "vdb_server 启动失败"
    exit 1
fi