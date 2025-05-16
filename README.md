# AmongVDB
## 相关依赖
### 安装BLAS库
```bash
# CentOS/RHEL
sudo yum install openblas-devel

# 或者 Ubuntu/Debian
# sudo apt-get install libopenblas-dev
```

### 引入faiss库
#### 方式一：使用包管理工具安装faiss库
使用以下命令安装faiss库：

```bash
# Ubuntu/Debian
sudo apt-get install libfaiss-dev

# CentOS/RHEL
sudo yum install faiss-devel
```

采用该方式时Makefile文件的faiss相关编译选项和链接选项如下：

```makefile
# 编译选项
CXXFLAGS = -std=c++11 -g -Wall

# 链接选项
LDFLAGS = -lfaiss -fopenmp -lopenblas -lpthread -lspdlog
```

#### 方式二：下载源代码编译faiss库（个人版）
由于faiss不是**header-only（仅由头文件组成）**的，不方便将源代码放入VDB工程项目文件夹中，为了便于在vscode编辑器中引用头文件，采用了“下载faiss源代码作为VDB工程项目文件夹的同级文件夹”的方式。

> 引用头文件方式见另一篇blog《无法引入FAISS库的头文件》
>

1. 下载命令：

```bash
# 从github下载faiss源码
cd <VDB工程项目的上级文件夹>
git clone https://github.com/facebookresearch/faiss
```

2. 更新Makefile文件：

```makefile
# 编译选项
CXXFLAGS = -std=c++11 -g -Wall -I../faiss -I../faiss/faiss

# 链接选项
LDFLAGS = -L../faiss/build/faiss -lfaiss -fopenmp -lopenblas -lpthread -lspdlog
```

> 如果不修改Makefile文件，可以在使用make命令编译完成后执行make install安装到系统目录，这两种方式的目的都是为了让VDB编译时能找到faiss库
>

3. 编译faiss库（前提是已经安装OpenBLAS）：

```bash
   cd ../faiss
   mkdir build
   cd build
   cmake ..
   make
```

此时可能会报错CUDA相关问题，解决方法有二：

```bash
# 方法1：禁用 CUDA 支持，只使用 CPU 版本
cd ../faiss_xx
rm -rf build
mkdir build
cd build
cmake .. -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF

# 方法2：安装 CUDA 工具包。如果您需要 GPU 支持，可以安装 CUDA：
# CentOS/RHEL
sudo yum install cuda-toolkit

# 或者从 NVIDIA 官网下载安装包
# https://developer.nvidia.com/cuda-downloads
```

解决CUDA相关问题后，可能仍报错google_benchmark相关问题，解决方法：

```bash
# 禁用性能测试
cd ../faiss_xx
rm -rf build
mkdir build
cd build
cmake .. -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF -DFAISS_ENABLE_BENCHMARK=OFF
```

解决完上述问题即可编译：

```bash
# 慢速
make
# 或开多线程加快编译：
make -j
```

### 引入httplib库、RapidJSON库、spdlog库
这几个库都是**header-only**的，从github上clone下来后把其中的include文件夹下的文件放进项目include文件夹对应目录即可

#### RapidJSON
```bash
# 先切换到 tmp 目录，然后在那里克隆 rapidjson
cd /tmp 
sudo git clone https://github.com/Tencent/rapidjson.git

# 复制到项目对应文件夹
cp -r /tmp/rapidjson/include/rapidjson/* /home/vdb/coding/AmongVDB/include/rapidjson/
```

#### Httplib
这个库只有一个头文件`httplib.h`，可以直接复制文件代码。这里就不放源码了。

[项目地址：https://github.com/yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib)

#### spdlog
```bash
# 先切换到 tmp 目录，然后在那里克隆 spdlog
cd /tmp 
sudo git clone https://github.com/gabime/spdlog.git

# 复制到项目对应文件夹
cp -r /tmp/spdlog/include/spdlog/* /home/vdb/coding/AmongVDB/include/spdlog/
```

#### 修改Makefile文件
增加查找以上库的路径

```makefile
# 编译选项
CXXFLAGS = -std=c++11 -g -Wall -I../faiss -I../faiss/faiss -I./include

# 链接选项
LDFLAGS = -L../faiss/build/faiss -lfaiss -fopenmp -lopenblas -lpthread
```

## 项目运行
```bash
# 项目编译
make

# 项目启动
./build/vdb_server

# 项目清理
make clean
```