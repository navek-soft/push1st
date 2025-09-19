# FindLua.cmake
# Locates the Lua development headers and library.
#
# Defines (standard CMake Find-module variables):
#   Lua_FOUND
#   Lua_INCLUDE_DIRS
#   Lua_LIBRARIES
#   Lua_VERSION_STRING   (best-effort, parsed from lua.h)
#
# Also defines:
#   Lua_INCLUDE_DIR
#   Lua_LIBRARY
#
# And provides an imported target:
#   Lua::Lua

# Hints & common suffixes for Debian/Ubuntu (liblua5.3-dev installs headers in /usr/include/lua5.3
# and the library as liblua5.3.so in a multi-arch lib dir)
set(_LUA_HEADER_NAMES lua.h)
set(_LUA_NAMES lua5.4 lua54 lua5.3 lua53 lua)

# --- ARM32 toolchain ---
if(PLATFORM_ARM32)
    find_path(Lua_INCLUDE_DIR
        NAMES ${_LUA_HEADER_NAMES}
        PATH_SUFFIXES lua5.4 lua5.3
        PATHS
            ${CMAKE_FIND_ROOT_PATH}/include
            ${CMAKE_FIND_ROOT_PATH}/usr/include
    )

    find_library(Lua_LIBRARY
        NAMES ${_LUA_NAMES}
        PATHS
            ${CMAKE_FIND_ROOT_PATH}/lib
            ${CMAKE_FIND_ROOT_PATH}/usr/lib
    )

# --- AArch64 toolchain ---
elseif(PLATFORM_AARCH64)
    set(TOOLCHAIN_PATH ${CMAKE_FIND_ROOT_PATH})

    find_path(Lua_INCLUDE_DIR
        NAMES ${_LUA_HEADER_NAMES}
        PATH_SUFFIXES lua5.4 lua5.3
        PATHS
            ${TOOLCHAIN_PATH}/include
            ${TOOLCHAIN_PATH}/usr/include
            /usr/include
    )

    find_library(Lua_LIBRARY
        NAMES ${_LUA_NAMES}
        PATHS
            ${TOOLCHAIN_PATH}/lib
            ${TOOLCHAIN_PATH}/usr/lib
            /usr/lib
    )

# --- Host / default ---
else()
    # Headers are often in /usr/include/lua5.3 (Debian/Ubuntu) or /usr/local/include
    find_path(Lua_INCLUDE_DIR
        NAMES ${_LUA_HEADER_NAMES}
        PATH_SUFFIXES lua5.4 lua5.3
        PATHS
            ${CMAKE_CURRENT_LIST_DIR}/../../../lua/include
            /usr/local/include
            /usr/include
    )

    # Libraries live in /usr/lib/<triplet> on Debian/Ubuntu; also check common spots
    find_library(Lua_LIBRARY
        NAMES ${_LUA_NAMES}
        PATHS
            ${CMAKE_CURRENT_LIST_DIR}/../../../lua/lib
            /usr/local/lib
            /usr/lib
            /usr/lib/x86_64-linux-gnu
            /usr/lib/aarch64-linux-gnu
            /usr/lib/arm-linux-gnueabihf
    )
endif()

# Try to extract a version from lua.h (optional)
set(Lua_VERSION_STRING "")
if(Lua_INCLUDE_DIR AND EXISTS "${Lua_INCLUDE_DIR}/lua.h")
    file(READ "${Lua_INCLUDE_DIR}/lua.h" _lua_h)
    string(REGEX MATCH "#define[ \t]+LUA_VERSION_MAJOR[ \t]+\"([0-9]+)\"" _m "${_lua_h}")
    if(CMAKE_MATCH_1)
        set(_LUA_VER_MAJOR "${CMAKE_MATCH_1}")
    endif()
    string(REGEX MATCH "#define[ \t]+LUA_VERSION_MINOR[ \t]+\"([0-9]+)\"" _n "${_lua_h}")
    if(CMAKE_MATCH_1)
        set(_LUA_VER_MINOR "${CMAKE_MATCH_1}")
    endif()
    if(_LUA_VER_MAJOR AND _LUA_VER_MINOR)
        set(Lua_VERSION_STRING "${_LUA_VER_MAJOR}.${_LUA_VER_MINOR}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Lua
    REQUIRED_VARS Lua_LIBRARY Lua_INCLUDE_DIR
    VERSION_VAR Lua_VERSION_STRING
)

if(Lua_FOUND)
    set(Lua_INCLUDE_DIRS "${Lua_INCLUDE_DIR}")
    set(Lua_LIBRARIES "${Lua_LIBRARY}")

    # Provide an imported target for modern usage
    if(NOT TARGET Lua::Lua)
        add_library(Lua::Lua UNKNOWN IMPORTED)
        set_target_properties(Lua::Lua PROPERTIES
            IMPORTED_LOCATION "${Lua_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Lua_INCLUDE_DIR}"
        )
    endif()
endif()

# Clean up internal vars
unset(_LUA_HEADER_NAMES)
unset(_LUA_NAMES)
unset(_m)
unset(_n)
unset(_LUA_VER_MAJOR)
unset(_LUA_VER_MINOR)
