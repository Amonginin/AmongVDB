#!/bin/bash

# AmongVDB 项目清理脚本
# 用于清理 build、ScalarStorage 和 WALLogStorage 文件夹

echo "开始清理 AmongVDB 项目文件夹..."
echo "========================================="

# 定义要清理的文件夹
FOLDERS_TO_CLEAN=("build" "ScalarStorage" "WALLogStorage")

# 获取脚本所在目录和项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "脚本路径: $SCRIPT_DIR"
echo "项目路径: $PROJECT_DIR"
echo ""

# 遍历每个需要清理的文件夹
for folder in "${FOLDERS_TO_CLEAN[@]}"; do
    folder_path="$PROJECT_DIR/$folder"
    
    if [ -d "$folder_path" ]; then
        echo "正在删除文件夹: $folder"
        rm -rf "$folder_path"
        
        if [ $? -eq 0 ]; then
            echo "✓ 成功删除: $folder"
        else
            echo "✗ 删除失败: $folder"
        fi
    else
        echo "- 文件夹不存在，跳过: $folder"
    fi
done

echo ""
echo "========================================="
echo "清理完成！"

# 显示剩余的文件夹结构
echo ""
echo "当前项目目录内容："
ls -la "$PROJECT_DIR" | grep "^d" | awk '{print $9}' | grep -v "^\.$" | grep -v "^\.\.$"