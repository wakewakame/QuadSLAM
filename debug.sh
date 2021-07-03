#!/bin/bash
set -e

TARGET="quadslam"
ARG="$HOME/Downloads/QuadVideos/2021-06-14_08-06-54"
BUILD_TYPE="Debug"

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
cp ./compile_commands.json ../
clear
make -j6
clear
if [ "$(uname)" == "Linux" ]; then
	./$BUILD_TYPE/$TARGET/$TARGET $ARG
elif [ "$(uname)" == "Darwin" ]; then
	./$BUILD_TYPE/$TARGET/$TARGET.app/Contents/MacOS/$TARGET $ARG
fi
