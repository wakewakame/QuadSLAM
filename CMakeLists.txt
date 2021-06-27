# CMakeの最小バージョンの指定
cmake_minimum_required(VERSION 3.13)

# プロジェクトの作成
project(QuadSLAM)

# C++17を使用する
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)

# compile_commands.jsonを生成するようにする
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "generate compile_commands.json" FORCE)

include(FetchContent)

# zlibの追加
string(TOLOWER "zlib" ZLIB_TARGET_NAME)
FetchContent_Declare(
	"${ZLIB_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/madler/zlib
	GIT_TAG v1.2.11
)
FetchContent_MakeAvailable("${ZLIB_TARGET_NAME}")
set(ZLIB_SOURCE_DIR "${${ZLIB_TARGET_NAME}_SOURCE_DIR}")

# OpenCVの追加
string(TOLOWER "opencv" OPENCV_TARGET_NAME)
# 必要最低限のモジュールのみをビルドする
set(OPENCV_BUILD_LIST
	"core"
	"imgproc"
	"imgcodecs"
	"highgui"
	"videoio"
)
string(REGEX REPLACE ";" "," OPENCV_BUILD_LIST_STRING "${OPENCV_BUILD_LIST}")
set(BUILD_LIST "${OPENCV_BUILD_LIST_STRING}" CACHE STRING "OpenCV Build List" FORCE)
FetchContent_Declare(
	"${OPENCV_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/opencv/opencv
	GIT_TAG 4.5.2
)
FetchContent_MakeAvailable("${OPENCV_TARGET_NAME}")
set(OPENCV_SOURCE_DIR "${${OPENCV_TARGET_NAME}_SOURCE_DIR}")
string(REGEX REPLACE "([^;]+)" "opencv_\\1" OPENCV_BUILD_LIST_TARGET "${OPENCV_BUILD_LIST}")
string(REGEX REPLACE "([^;]+)" "${OPENCV_SOURCE_DIR}/modules/\\1/include" OPENCV_BUILD_LIST_INCLUDE "${OPENCV_BUILD_LIST}")

# nlohmann_jsonの追加
string(TOLOWER "nlohmann_json" JSON_TARGET_NAME)
# テストはビルドしないようにする
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
	"${JSON_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/nlohmann/json
	GIT_TAG v3.9.1
)
FetchContent_MakeAvailable("${JSON_TARGET_NAME}")
set(JSON_SOURCE_DIR "${${JSON_TARGET_NAME}_SOURCE_DIR}")
set(JSON_BINARY_DIR "${${JSON_TARGET_NAME}_BINARY_DIR}")

# Cinderの追加
string(TOLOWER "cinder" CINDER_TARGET_NAME)
FetchContent_Declare(
	"${CINDER_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/cinder/Cinder
	GIT_TAG 374c8834f2e03c476d1d913486dd6a5038dceba6
)
FetchContent_MakeAvailable("${CINDER_TARGET_NAME}")
set(CINDER_SOURCE_DIR "${${CINDER_TARGET_NAME}_SOURCE_DIR}")
include("${CINDER_SOURCE_DIR}/proj/cmake/modules/cinderMakeApp.cmake")

# 全てのソースファイルを再帰的に検索する
file(GLOB_RECURSE ALL_CPP_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# 実行ファイルを作成するための関数
function(qs_make_app)
	# 引数の解析
	set(oneValueArgs APP_NAME SOURCE)
	cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

	# ターゲットの作成
	ci_make_app(
		CINDER_PATH "${CINDER_SOURCE_DIR}"
		APP_NAME    "${ARG_APP_NAME}"
		SOURCES     "${ARG_SOURCE}" "${ALL_CPP_FILES}"
		INCLUDES
			"${PROJECT_SOURCE_DIR}/include"
			"${ZLIB_SOURCE_DIR}"
			"${CMAKE_BINARY_DIR}"
			"${OPENCV_SOURCE_DIR}/include"
			"${OPENCV_BUILD_LIST_INCLUDE}"
			 "${JSON_SOURCE_DIR}/include"
		LIBRARIES
			"${ZLIB_TARGET_NAME}"
			"${OPENCV_BUILD_LIST_TARGET}"
	)
endfunction()

# メインプログラムの作成
qs_make_app(APP_NAME quadslam SOURCE "${PROJECT_SOURCE_DIR}/main.cpp")

# 全サンプルプログラムの作成
file(GLOB_RECURSE ALL_EXAMPLE_FILES "${PROJECT_SOURCE_DIR}/examples/*.cpp")
foreach(SOURCE IN LISTS ALL_EXAMPLE_FILES)
	get_filename_component(EXAMPLE_NAME "${SOURCE}" NAME_WLE)
	qs_make_app(APP_NAME "example_${EXAMPLE_NAME}" SOURCE "${SOURCE}")
endforeach()
