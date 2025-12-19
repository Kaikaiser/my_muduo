# 要求最低cmake版本
cmake_minimum_required(VERSION 3.10) 
project(muduo_mine)

# cmake -> makefile -> make
# muduo_mine最终编译成so动态库，设置动态库的路径，放在根目录的lib目录下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置调试（编译）信息 启动c++语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

#定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/http HTTP_SRC)
list(APPEND SRC_LIST ${HTTP_SRC})

#编译生成动态库muduo_mine
add_library(muduo_mine SHARED ${SRC_LIST})