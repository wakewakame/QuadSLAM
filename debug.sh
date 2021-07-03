#!/bin/bash
set -e

#TARGET="quadslam"
TARGET="example_cinder_pointcloud"
#TARGET="example_cinder_preview"
ARG="$HOME/Downloads/QuadVideos/2021-06-14_09-00-38"
#ARG="$HOME/Downloads/QuadVideos/2021-06-14_08-06-54"
BUILD_TYPE="Debug"

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
cp ./compile_commands.json ../
clear
make -j6
clear

# 例外発生時にcoreファイルをダンプするようにする
# coreファイルは`gdb <実行ファイルのパス> <coreのパス>`で例外発生時の状況を確認できる
ulimit -c unlimited

if [ "$(uname)" == "Linux" ]; then
	./$BUILD_TYPE/$TARGET/$TARGET $ARG
elif [ "$(uname)" == "Darwin" ]; then
	./$BUILD_TYPE/$TARGET/$TARGET.app/Contents/MacOS/$TARGET $ARG
fi
