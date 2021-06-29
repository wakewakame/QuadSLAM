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

include(ExternalProject)
include(FetchContent)

# zlibの追加
string(TOLOWER "zlib" ZLIB_TARGET_NAME)
set(ZLIB_DIR "${PROJECT_BINARY_DIR}/3rdparty/${ZLIB_TARGET_NAME}")
set(ZLIB_SOURCE_DIR "${ZLIB_DIR}/Source/${ZLIB_TARGET_NAME}")
set(ZLIB_BINARY_DIR "${ZLIB_DIR}/Build/${ZLIB_TARGET_NAME}")
ExternalProject_Add(
	"${ZLIB_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/madler/zlib
	GIT_TAG v1.2.11
	PREFIX "${ZLIB_DIR}"
	SOURCE_DIR "${ZLIB_SOURCE_DIR}"
	BINARY_DIR "${ZLIB_BINARY_DIR}"
	#CMAKE_GENERATOR "Unix Makefiles"
	#BUILD_COMMAND make
	INSTALL_COMMAND ""
)

# OpenCVの追加
string(TOLOWER "opencv" OPENCV_TARGET_NAME)
set(OPENCV_DIR "${PROJECT_BINARY_DIR}/3rdparty/${OPENCV_TARGET_NAME}")
set(OPENCV_SOURCE_DIR "${OPENCV_DIR}/Source/${OPENCV_TARGET_NAME}")
set(OPENCV_BINARY_DIR "${OPENCV_DIR}/Build/${OPENCV_TARGET_NAME}")
set(OPENCV_BUILD_LIST
	"core"
	"imgproc"
	"imgcodecs"
	"highgui"
	"videoio"
)
string(REGEX REPLACE ";" "," OPENCV_BUILD_LIST_STRING "${OPENCV_BUILD_LIST};")
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	string(REGEX REPLACE "([^;]+)" "libopencv_\\1.dylib" OPENCV_BUILD_LIST_LIBRARIES "${OPENCV_BUILD_LIST}")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	string(REGEX REPLACE "([^;]+)" "libopencv_\\1.so" OPENCV_BUILD_LIST_LIBRARIES "${OPENCV_BUILD_LIST}")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
	string(REGEX REPLACE "([^;]+)" "libopencv_\\1.so" OPENCV_BUILD_LIST_LIBRARIES "${OPENCV_BUILD_LIST}")
else()
	message(FATAL_ERROR "this operating system is not supported.")
endif()
string(REGEX REPLACE "([^;]+)" "${OPENCV_SOURCE_DIR}/modules/\\1/include" OPENCV_BUILD_LIST_INCLUDE "${OPENCV_BUILD_LIST}")
ExternalProject_Add(
	"${OPENCV_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/opencv/opencv
	GIT_TAG 4.5.2
	PREFIX "${OPENCV_DIR}"
	SOURCE_DIR "${OPENCV_SOURCE_DIR}"
	BINARY_DIR "${OPENCV_BINARY_DIR}"
	#CMAKE_GENERATOR "Unix Makefiles"
	#BUILD_COMMAND make
	INSTALL_COMMAND ""
	CMAKE_ARGS
		-DBUILD_LIST="${OPENCV_BUILD_LIST_STRING}"
		-DBUILD_ZLIB=ON
)

# nlohmann_jsonの追加
string(TOLOWER "nlohmann_json" JSON_TARGET_NAME)
set(JSON_DIR "${PROJECT_BINARY_DIR}/3rdparty/${JSON_TARGET_NAME}")
set(JSON_SOURCE_DIR "${JSON_DIR}/Source/${JSON_TARGET_NAME}")
set(JSON_BINARY_DIR "${JSON_DIR}/Build/${JSON_TARGET_NAME}")
ExternalProject_Add(
	"${JSON_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/nlohmann/json
	GIT_TAG v3.9.1
	PREFIX "${JSON_DIR}"
	SOURCE_DIR "${JSON_SOURCE_DIR}"
	BINARY_DIR "${JSON_BINARY_DIR}"
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	CMAKE_ARGS -DJSON_BuildTests=OFF
)

# Cinderの追加
string(TOLOWER "Cinder" CINDER_TARGET_NAME)
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
	)

	# 以下はCinderを使用しない場合のターゲット追加方法
	# ターゲットの作成
	#add_executable("${ARG_APP_NAME}" "${ARG_SOURCE}" "${ALL_CPP_FILES}")

	# 追加のインクルードディレクトリ
	target_include_directories("${ARG_APP_NAME}" PUBLIC "${PROJECT_SOURCE_DIR}/include")

	# zlibをライブラリとして追加
	add_dependencies("${ARG_APP_NAME}" "${ZLIB_TARGET_NAME}")
	target_include_directories("${ARG_APP_NAME}" PUBLIC "${ZLIB_SOURCE_DIR}")
	target_link_directories("${ARG_APP_NAME}" PUBLIC "${ZLIB_BINARY_DIR}")
	if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
		target_link_libraries("${ARG_APP_NAME}" "libz.a")
	elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
		target_link_libraries("${ARG_APP_NAME}" "libz.a")
	elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
		target_link_libraries("${ARG_APP_NAME}" "libz.lib")
	endif()

	# OpenCVをライブラリとして追加
	add_dependencies("${ARG_APP_NAME}" "${OPENCV_TARGET_NAME}")
	# 追加のライブラリディレクトリ
	target_link_directories("${ARG_APP_NAME}" PUBLIC "${OPENCV_BINARY_DIR}/lib")
	# 追加の依存ファイル
	target_link_libraries("${ARG_APP_NAME}" "${OPENCV_BUILD_LIST_LIBRARIES}")
	target_include_directories("${ARG_APP_NAME}" PUBLIC
		"${OPENCV_SOURCE_DIR}/include"
		"${OPENCV_BUILD_LIST_INCLUDE}"
		"${OPENCV_BINARY_DIR}"
	)

	# nlohmann_jsonをライブラリとして追加
	add_dependencies("${ARG_APP_NAME}" "${JSON_TARGET_NAME}")
	target_include_directories("${ARG_APP_NAME}" PRIVATE "${JSON_SOURCE_DIR}/include")
endfunction()

# メインプログラムの作成
qs_make_app(APP_NAME quadslam SOURCE "${PROJECT_SOURCE_DIR}/main.cpp")

# 全サンプルプログラムの作成
file(GLOB_RECURSE ALL_EXAMPLE_FILES "${PROJECT_SOURCE_DIR}/examples/*.cpp")
foreach(SOURCE IN LISTS ALL_EXAMPLE_FILES)
	get_filename_component(EXAMPLE_NAME "${SOURCE}" NAME_WLE)
	qs_make_app(APP_NAME "example_${EXAMPLE_NAME}" SOURCE "${SOURCE}")
endforeach()
