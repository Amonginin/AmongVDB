#!/bin/bash

echo "======================"
echo "开始编译流程"
echo "======================"

# 执行make时动态获取系统线程数
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

# 解决启动编译好的vdb_server时无法加载动态链接库的问题
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/../rocksdb:$(pwd)/../NuRaft/build
echo "已设置 LD_LIBRARY_PATH: $LD_LIBRARY_PATH"

# 启动vdb_server
echo "启动 vdb_server..."
if ./build/vdb_server; then
    echo "vdb_server 启动成功"
else
    echo "vdb_server 启动失败"
    exit 1
fi