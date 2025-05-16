# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -std=c++11 -g -Wall -I../faiss -I../faiss/faiss -I./include

# 链接选项
LDFLAGS = -L../faiss/build/faiss -lfaiss -fopenmp -lopenblas -lpthread

# 目标文件
TARGET = build/vdb_server

# 源文件
SOURCES = vdb_server.cpp faiss_index.cpp http_server.cpp index_factory.cpp logger.cpp

# 对象文件
OBJECTS = $(SOURCES:%.cpp=build/%.o)

# 创建 build 目录
$(shell mkdir -p build)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

build/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build