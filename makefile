# 编译器
CXX = g++

# 编译选项
# 将INCLUDES添加到CXXFLAGS
CXXFLAGS = -std=c++17 -g -Wall $(INCLUDES)

# 链接选项
LDFLAGS = -fopenmp -L../faiss/build/faiss -lfaiss -L../rocksdb -lrocksdb -lopenblas -lpthread -L../CRoaring/build/src -lroaring

# Include 目录
INCLUDES = -I./include -I../faiss -I../rocksdb/include -I../CRoaring/include

# 目标文件
TARGET = build/vdb_server

# 源文件
SOURCES = vdb_server.cpp faiss_index.cpp http_server.cpp index_factory.cpp \
logger.cpp hnswlib_index.cpp scalar_storage.cpp vector_database.cpp filter_index.cpp

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