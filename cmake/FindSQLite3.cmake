message(STATUS "hogeeeeeeeeeeeeeeeeeee")

find_path(SQLite3_INCLUDE_DIR NAMES sqlite3.h HINTS ${CMAKE_CURRENT_SOURCE_DIR}/build/3rdparty/sqlite3/Install/sqlite3/include)
mark_as_advanced(SQLite3_INCLUDE_DIR)
find_library(SQLite3_LIBRARY NAMES sqlite3 sqlite HINTS ${CMAKE_CURRENT_SOURCE_DIR}/build/3rdparty/sqlite3/Install/sqlite3/lib)
mark_as_advanced(SQLite3_LIBRARY)

if(SQLite3_INCLUDE_DIR)
    file(STRINGS ${SQLite3_INCLUDE_DIR}/sqlite3.h _ver_line
         REGEX "^#define SQLITE_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
         LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
           SQLite3_VERSION "${_ver_line}")
    unset(_ver_line)
endif()

find_package_handle_standard_args(SQLite3
    REQUIRED_VARS SQLite3_INCLUDE_DIR SQLite3_LIBRARY
    VERSION_VAR SQLite3_VERSION
)

if(SQLite3_FOUND)
    set(SQLite3_INCLUDE_DIRS ${SQLite3_INCLUDE_DIR})
    set(SQLite3_LIBRARIES ${SQLite3_LIBRARY})
    if(NOT TARGET SQLite::SQLite3)
        add_library(SQLite::SQLite3 UNKNOWN IMPORTED)
        set_target_properties(SQLite::SQLite3 PROPERTIES
            IMPORTED_LOCATION             "${SQLite3_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIR}")
    endif()
endif()