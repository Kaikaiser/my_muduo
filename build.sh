#!/bin/bash


set -e

# 如果没有build目录 创建build目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/* 

cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录下
cd ..

# 把头文件拷贝到 /usr/include/muduomy  so库拷贝到/usr/lib  PATH
if [ ! -d /usr/include/muduomy ]; then
    mkdir /usr/include/muduomy
fi

for header in `ls *.h`
do
    cp $header /usr/include/muduomy
done

cp `pwd`/lib/muduomy.so /usr/lib

# 刷新 
ldconfig