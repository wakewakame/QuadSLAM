# CMakeの最小バージョンの指定
cmake_minimum_required(VERSION 3.10)

# プロジェクトの作成
project(QuadSLAM)

# C++17を使用する
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)

# compile_commands.jsonを生成するようにする
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "generate compile_commands.json" FORCE)

# 依存ライブラリのパス
set(OPENCV_DIR "${PROJECT_SOURCE_DIR}/3rdparty/opencv")
set(JSON_DIR "${PROJECT_SOURCE_DIR}/3rdparty/json")

# OpenCVで使用するModuleの指定
set(OPENCV_BUILD_LIST
	"core"
	"imgproc"
	"imgcodecs"
	"highgui"
	"videoio"
)
string(REGEX REPLACE ";" "," OPENCV_BUILD_LIST_STRING "${OPENCV_BUILD_LIST}")
string(REGEX REPLACE "([^;]+)" "opencv_\\1" OPENCV_BUILD_LIST_TARGET "${OPENCV_BUILD_LIST}")
string(REGEX REPLACE "([^;]+)" "${OPENCV_DIR}/modules/\\1/include" OPENCV_BUILD_LIST_INCLUDE "${OPENCV_BUILD_LIST}")

# OpenCVの中で必要なモジュールのみビルドするように設定
set(BUILD_LIST "${OPENCV_BUILD_LIST_STRING}" CACHE STRING "OpenCV Build List" FORCE)

# 本プログラムではzlibを使用するが、これはOpenCVが既に使用している。
# そのため、自前でzlibを追加するとOpenCVの使用するzlibとターゲット名の衝突を起こす。
# これを回避するべく、ここではOpenCVに含まれるzlibを利用することにした。

# OpenCVでzlibの自前ビルドを強制する
# zlibのターゲット名はZLIB_LIBRARY変数にCACHEとして格納される
# zlibのインクルードディレクトリはZLIB_INCLUDE_DIR変数にCACHEとして格納される
set(BUILD_ZLIB ON CACHE INTERNAL "" FORCE)

# nlohmann_jsonのビルドテストは行わない
set(JSON_BuildTests OFF CACHE INTERNAL "")

# 依存ライブラリのCMakeLists.txtを読み込む
add_subdirectory("${OPENCV_DIR}")
add_subdirectory("${JSON_DIR}")

# 全てのソースファイルを再帰的に検索する
file(GLOB_RECURSE ALL_CPP_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# メインプログラムの作成
add_executable(quadslam "${PROJECT_SOURCE_DIR}/main.cpp" "${ALL_CPP_FILES}")
target_include_directories(quadslam PUBLIC "${PROJECT_SOURCE_DIR}/include")

# OpenCVをライブラリとして追加
target_link_libraries(quadslam PRIVATE "${OPENCV_BUILD_LIST_TARGET}")
target_include_directories(quadslam PRIVATE
	"${CMAKE_BINARY_DIR}"
	"${OPENCV_DIR}/include"
	"${OPENCV_BUILD_LIST_INCLUDE}"
)

# zlibをライブラリとして追加
target_link_libraries(quadslam PRIVATE "${ZLIB_LIBRARY}")
target_include_directories(quadslam PRIVATE "${ZLIB_INCLUDE_DIR}")

# nlohmann_jsonをライブラリとして追加
target_link_libraries(quadslam PRIVATE nlohmann_json::nlohmann_json)
