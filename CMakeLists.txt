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

# OpenCVとCinderはともにzlibに依存しており、このプログラムにはzlibというターゲット名が2つ存在することになる。
# しかし、CMakeはターゲット名の重複を許していない
# これを回避するため、ここではExternalProjectで依存ライブラリを管理する。
# ただし、ExternalProjectはcmakeコマンド実行時ではなくビルド時(makeを実行したときなど)にリポジトリをクローンする。
# そのため、依存ライブラリのリポジトリ内にある*.cmakeファイルなどをここでincludeできない。
# Cinderのリポジトリ内にincludeしたいcmakeファイルがあるため、ここでは例外的にCinderをFetchContentで追加している。

# OpenCVの追加
string(TOLOWER "opencv" OPENCV_TARGET_NAME)
set(OPENCV_VERSION "4.5.3")
set(OPENCV_DIR "${PROJECT_BINARY_DIR}/3rdparty/${OPENCV_TARGET_NAME}")
set(OPENCV_SOURCE_DIR "${OPENCV_DIR}/Source/${OPENCV_TARGET_NAME}")
set(OPENCV_BINARY_DIR "${OPENCV_DIR}/Build/${OPENCV_TARGET_NAME}")
set(OPENCV_OUTPUT_DIR "${OPENCV_DIR}/Output/${OPENCV_TARGET_NAME}")
set(OPENCV_BUILD_LIST
	"core"
	"imgproc"
	"imgcodecs"
	"highgui"
	"videoio"
)
string(REGEX REPLACE ";" "," OPENCV_BUILD_LIST_STRING "${OPENCV_BUILD_LIST};")
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	string(REGEX REPLACE "\\." "" OPENCV_VERSION_ "${OPENCV_VERSION}")
	string(REGEX REPLACE "([^;]+)" "opencv_\\1${OPENCV_VERSION_}" OPENCV_BUILD_LIST_LIBRARIES "${OPENCV_BUILD_LIST}")
else()
	string(REGEX REPLACE "([^;]+)" "libopencv_\\1.${OPENCV_VERSION}" OPENCV_BUILD_LIST_LIBRARIES "${OPENCV_BUILD_LIST}")
endif()
string(REGEX REPLACE "([^;]+)" "${OPENCV_SOURCE_DIR}/modules/\\1/include" OPENCV_BUILD_LIST_INCLUDE "${OPENCV_BUILD_LIST}")
ExternalProject_Add(
	"${OPENCV_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/opencv/opencv.git
	GIT_TAG ${OPENCV_VERSION}
	UPDATE_DISCONNECTED YES
	PREFIX "${OPENCV_DIR}"
	SOURCE_DIR "${OPENCV_SOURCE_DIR}"
	BINARY_DIR "${OPENCV_BINARY_DIR}"
	INSTALL_COMMAND ""
	CMAKE_ARGS
		-DBUILD_LIST="${OPENCV_BUILD_LIST_STRING}"
)

# SQLiteCppの追加
#string(TOLOWER "SQLiteCpp" SQLITE_TARGET_NAME)
#set(SQLITE_DIR "${PROJECT_BINARY_DIR}/3rdparty/${SQLITE_TARGET_NAME}")
#set(SQLITE_SOURCE_DIR "${SQLITE_DIR}/Source/${SQLITE_TARGET_NAME}")
#set(SQLITE_BINARY_DIR "${SQLITE_DIR}/Build/${SQLITE_TARGET_NAME}")
#ExternalProject_Add(
#	"${SQLITE_TARGET_NAME}"
#	GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
#	GIT_TAG 0c46d86e0d82031924c5d10bf9b23fa917ba037e
#	GIT_CONFIG "core.autocrlf=input"
#	UPDATE_DISCONNECTED YES
#	PREFIX "${SQLITE_DIR}"
#	SOURCE_DIR "${SQLITE_SOURCE_DIR}"
#	BINARY_DIR "${SQLITE_BINARY_DIR}"
#	INSTALL_COMMAND ""
#	CMAKE_ARGS
#		-DSQLITECPP_USE_STATIC_RUNTIME=ON
#)

# sqlite3の追加
string(TOLOWER "qs_sqlite3" SQLITE_TARGET_NAME)
set(SQLITE_DIR ${PROJECT_BINARY_DIR}/3rdparty/${SQLITE_TARGET_NAME})
set(SQLITE_INSTALL_DIR ${SQLITE_DIR}/Install/${SQLITE_TARGET_NAME})
set(SQLITE_FILENAME ${CMAKE_STATIC_LIBRARY_PREFIX}sqlite3${CMAKE_STATIC_LIBRARY_SUFFIX})
ExternalProject_Add(
	"${SQLITE_TARGET_NAME}"
	URL "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sqlite3"
	UPDATE_DISCONNECTED NO
	PREFIX "${SQLITE_DIR}"
	CMAKE_ARGS
		-DCMAKE_INSTALL_PREFIX=${SQLITE_INSTALL_DIR}
)

# sqlite_ormの追加
string(TOLOWER "sqlite_orm" SQLITE_ORM_TARGET_NAME)
set(SQLITE_ORM_DIR "${PROJECT_BINARY_DIR}/3rdparty/${SQLITE_ORM_TARGET_NAME}")
set(SQLITE_ORM_SOURCE_DIR "${SQLITE_ORM_DIR}/Source/${SQLITE_ORM_TARGET_NAME}")
ExternalProject_Add(
	"${SQLITE_ORM_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/fnc12/sqlite_orm.git
	GIT_TAG 1ee0a8653fe57ed4d4f69b5a65839b1861c41d32
	UPDATE_DISCONNECTED NO
	PREFIX "${SQLITE_ORM_DIR}"
	SOURCE_DIR "${SQLITE_ORM_SOURCE_DIR}"
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	CMAKE_ARGS
		-DCMAKE_MODULE_PATH=${CMAKE_CURRENT_SOURCE_DIR}/cmake
		-DQUADSLAM_SQLITE_INCLUDE=${SQLITE_INSTALL_DIR}/include
		-DQUADSLAM_SQLITE_LIBRARY=${SQLITE_INSTALL_DIR}/lib/${SQLITE_FILENAME}
		-DBUILD_EXAMPLES=OFF
)
add_dependencies("${SQLITE_ORM_TARGET_NAME}" "${SQLITE_TARGET_NAME}")

# nlohmann_jsonの追加
string(TOLOWER "nlohmann_json" JSON_TARGET_NAME)
set(JSON_DIR "${PROJECT_BINARY_DIR}/3rdparty/${JSON_TARGET_NAME}")
set(JSON_SOURCE_DIR "${JSON_DIR}/Source/${JSON_TARGET_NAME}")
ExternalProject_Add(
	"${JSON_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/nlohmann/json.git
	GIT_TAG v3.9.1
	UPDATE_DISCONNECTED YES
	PREFIX "${JSON_DIR}"
	SOURCE_DIR "${JSON_SOURCE_DIR}"
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	CMAKE_ARGS
		-DJSON_BuildTests=OFF
)

# zlibの追加
string(TOLOWER "qs_zlib" ZLIB_TARGET_NAME)
set(ZLIB_DIR "${PROJECT_BINARY_DIR}/3rdparty/${ZLIB_TARGET_NAME}")
set(ZLIB_SOURCE_DIR "${ZLIB_DIR}/Source/${ZLIB_TARGET_NAME}")
set(ZLIB_BINARY_DIR "${ZLIB_DIR}/Build/${ZLIB_TARGET_NAME}")
set(ZLIB_INCLUDE_DIR "${ZLIB_DIR}/Source" "${ZLIB_BINARY_DIR}")
ExternalProject_Add(
	"${ZLIB_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/madler/zlib.git
	GIT_TAG v1.2.11
	UPDATE_DISCONNECTED YES
	PREFIX "${ZLIB_DIR}"
	SOURCE_DIR "${ZLIB_SOURCE_DIR}"
	BINARY_DIR "${ZLIB_BINARY_DIR}"
	INSTALL_COMMAND ""
)

# Cinderの追加
string(TOLOWER "Cinder" CINDER_TARGET_NAME)
FetchContent_Declare(
	"${CINDER_TARGET_NAME}"
	GIT_REPOSITORY https://github.com/cinder/Cinder.git
	GIT_TAG 374c8834f2e03c476d1d913486dd6a5038dceba6
)
FetchContent_MakeAvailable("${CINDER_TARGET_NAME}")
set(CINDER_SOURCE_DIR "${${CINDER_TARGET_NAME}_SOURCE_DIR}")
include("${CINDER_SOURCE_DIR}/proj/cmake/modules/cinderMakeApp.cmake")

# 全てのソースファイルを再帰的に検索する
file(GLOB_RECURSE ALL_CPP_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

function(qs_target_link_libraries_plain_name TARGET)
	set(LIBS "${ARGN}")
	if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
		string(REGEX REPLACE "([^;]+)" "lib\\1.dylib" LIBS "${LIBS}")
	elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
		string(REGEX REPLACE "([^;]+)" "lib\\1.so" LIBS "${LIBS}")
	elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
		string(REGEX REPLACE "([^;]+)" "\\1$<$<CONFIG:Debug>:d>.lib" LIBS "${LIBS}")
	else()
		message(FATAL_ERROR "this operating system is not supported.")
	endif()
	target_link_libraries("${TARGET}" "${LIBS}")
endfunction()

# 実行ファイルを作成するためのマクロ
macro(qs_make_app)
	# 引数の解析
	set(oneValueArgs APP_NAME SOURCE ENABLE_CINDER)
	cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

	# ターゲットの作成
	if(${ARG_ENABLE_CINDER})
		ci_make_app(
			CINDER_PATH "${CINDER_SOURCE_DIR}"
			APP_NAME    "${ARG_APP_NAME}"
			SOURCES     "${ARG_SOURCE}" "${ALL_CPP_FILES}"
		)
	else()
		add_executable("${ARG_APP_NAME}" "${ARG_SOURCE}" "${ALL_CPP_FILES}")
	endif()

	# 追加のインクルードディレクトリ
	target_include_directories("${ARG_APP_NAME}" PUBLIC "${PROJECT_SOURCE_DIR}/include")

	# 追加のライブラリ
	target_link_libraries("${ARG_APP_NAME}" Threads::Threads)

	# zlibをライブラリとして追加
	add_dependencies("${ARG_APP_NAME}" "${ZLIB_TARGET_NAME}")
	target_link_directories("${ARG_APP_NAME}" PUBLIC "${ZLIB_BINARY_DIR}")
	target_include_directories("${ARG_APP_NAME}" PRIVATE "${ZLIB_INCLUDE_DIR}")
	qs_target_link_libraries_plain_name("${ARG_APP_NAME}" "zlibstatic")

	# OpenCVをライブラリとして追加
	add_dependencies("${ARG_APP_NAME}" "${OPENCV_TARGET_NAME}")
	target_link_directories("${ARG_APP_NAME}" PUBLIC "${OPENCV_BINARY_DIR}/lib")
	qs_target_link_libraries_plain_name("${ARG_APP_NAME}" "${OPENCV_BUILD_LIST_LIBRARIES}")
	target_include_directories("${ARG_APP_NAME}" PUBLIC
		"${OPENCV_SOURCE_DIR}/include"
		"${OPENCV_BUILD_LIST_INCLUDE}"
		"${OPENCV_BINARY_DIR}"
	)

	# SQLiteCppをライブラリとして追加
	#add_dependencies("${ARG_APP_NAME}" "${SQLITE_TARGET_NAME}")
	#target_link_directories("${ARG_APP_NAME}" PUBLIC
	#	"${SQLITE_BINARY_DIR}"
	#	"${SQLITE_BINARY_DIR}/sqlite3"
	#)
	#target_link_libraries("${ARG_APP_NAME}" "sqlite3" "SQLiteCpp")
	#target_include_directories("${ARG_APP_NAME}" PUBLIC "${SQLITE_SOURCE_DIR}/include")

	# sqliteをライブラリとして追加
	add_dependencies("${ARG_APP_NAME}" "${SQLITE_TARGET_NAME}")
	target_link_directories("${ARG_APP_NAME}" PUBLIC "${SQLITE_INSTALL_DIR}/lib")
	target_link_libraries("${ARG_APP_NAME}" "sqlite3")
	target_include_directories("${ARG_APP_NAME}" PRIVATE "${SQLITE_INSTALL_DIR}/include")

	# sqlite_ormをライブラリとして追加
	add_dependencies("${ARG_APP_NAME}" "${SQLITE_ORM_TARGET_NAME}")
	target_include_directories("${ARG_APP_NAME}" PRIVATE "${SQLITE_ORM_SOURCE_DIR}/include")

	# nlohmann_jsonをライブラリとして追加
	add_dependencies("${ARG_APP_NAME}" "${JSON_TARGET_NAME}")
	target_include_directories("${ARG_APP_NAME}" PRIVATE "${JSON_SOURCE_DIR}/include")

	# Windows用の設定
	if(CMAKE_SYSTEM_NAME MATCHES "Windows")
		if (MSVC)
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
		endif()

		# CP932からUTF-8に変換するときに使う
		target_link_libraries("${ARG_APP_NAME}" "Kernel32.lib")

		# デバッグ実行時に依存dllを読めるように環境を設定
		set_target_properties("${ARG_APP_NAME}" PROPERTIES VS_DEBUGGER_ENVIRONMENT
			"PATH=%PATH%;${OPENCV_BINARY_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>"
		)
	endif()
endmacro()

# メインプログラムの作成
qs_make_app(APP_NAME quadslam SOURCE "${PROJECT_SOURCE_DIR}/main.cpp" ENABLE_CINDER OFF)

# 全サンプルプログラムの作成
file(GLOB_RECURSE ALL_CLI_EXAMPLE_FILES "${PROJECT_SOURCE_DIR}/examples/cli/*.cpp")
foreach(SOURCE IN LISTS ALL_CLI_EXAMPLE_FILES)
	get_filename_component(EXAMPLE_NAME "${SOURCE}" NAME_WLE)
	qs_make_app(APP_NAME "example_${EXAMPLE_NAME}" SOURCE "${SOURCE}" ENABLE_CINDER OFF)
endforeach()
file(GLOB_RECURSE ALL_CINDER_EXAMPLE_FILES "${PROJECT_SOURCE_DIR}/examples/cinder/*.cpp")
foreach(SOURCE IN LISTS ALL_CINDER_EXAMPLE_FILES)
	get_filename_component(EXAMPLE_NAME "${SOURCE}" NAME_WLE)
	qs_make_app(APP_NAME "example_${EXAMPLE_NAME}" SOURCE "${SOURCE}" ENABLE_CINDER ON)
endforeach()
