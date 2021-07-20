file(
	STRINGS ${QUADSLAM_SQLITE_INCLUDE}/sqlite3.h _ver_line
	REGEX "^#define SQLITE_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
	LIMIT_COUNT 1
)
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" SQLite3_VERSION "${_ver_line}")
unset(_ver_line)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3
	REQUIRED_VARS QUADSLAM_SQLITE_INCLUDE QUADSLAM_SQLITE_LIBRARY
	VERSION_VAR SQLite3_VERSION
)

if(SQLite3_FOUND)
	set(SQLite3_INCLUDE_DIRS ${QUADSLAM_SQLITE_INCLUDE})
	set(SQLite3_LIBRARIES ${QUADSLAM_SQLITE_LIBRARY})
	if(NOT TARGET SQLite::SQLite3)
		add_library(SQLite::SQLite3 UNKNOWN IMPORTED)
		set_target_properties(SQLite::SQLite3 PROPERTIES
			IMPORTED_LOCATION "${QUADSLAM_SQLITE_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${QUADSLAM_SQLITE_INCLUDE}")
	endif()
endif()
