cmake_minimum_required(VERSION 3.16)

project(push1st)

include(cmake/module.cmake)
enable_testing()

string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)
set(PATCH "0" CACHE STRING "Patch")
set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(BUILD_NO "0" CACHE STRING "Build no")
set(VERSION_MINOR "0" CACHE STRING "Minor version")
set(VERSION_MAJOR "default" CACHE STRING "Major version")
set(PERFORMANCE_TRACE OFF CACHE BOOL "Performance tracing report")
set(CLANG_TIDY_CHECK ON CACHE BOOL "clang-tidy source code check")
set(APP_NAME "push1st" CACHE STRING "App name")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS_DEBUG "-g3 -DDEBUG -flto=auto -fno-omit-frame-pointer -std=c++2a -ggdb3 -O0 -Wall -Wextra -Werror -Wno-unused-result -Wno-deprecated-declarations -fno-inline")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto=auto -std=c++2a -Wall -Wextra -Werror -Wno-deprecated-declarations -Wno-unused-result")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -ggdb3 -flto=auto -std=c++2a -Wall -Wextra -Werror -Wno-deprecated-declarations -Wno-unused-result")

if(PERFORMANCE_TRACE)
        set(BUILD_FLAGS "${BUILD_FLAGS}[perfomance trace] ")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DPERFORMANCE_TRACE")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DPERFORMANCE_TRACE")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DPERFORMANCE_TRACE")
endif()

if(CLANG_TIDY_CHECK AND NOT PERFORMANCE_TRACE)
        find_program(CLANG_TIDY clang-tidy-15)

        if(NOT CLANG_TIDY)
                message(FATAL_ERROR "clang-tidy-15 is not found! Please install it.")
        endif()

        set(CLANG_TIDY_CACHE ${CMAKE_CURRENT_SOURCE_DIR}/script/clang_tidy_cache.py)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_CACHE};${CLANG_TIDY};-format-style=file;-header-filter=^(${CMAKE_CURRENT_SOURCE_DIR}/application|${CMAKE_CURRENT_SOURCE_DIR}/source|${CMAKE_CURRENT_SOURCE_DIR}/include)/;-config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
file(STRINGS "/etc/os-release" OS_RELEASE_CONTENT REGEX "VERSION_ID")
string(REGEX MATCH "[0-9]+\.[0-9]+" OS_VERSION_ID ${OS_RELEASE_CONTENT})
file(STRINGS "/etc/os-release" OS_RELEASE_CONTENT REGEX "PRETTY_NAME")
string(REGEX MATCH "\".+\"" OS_PRETTY_NAME ${OS_RELEASE_CONTENT})

message(STATUS "BUILD_TYPE = ${BUILD_TYPE}")
message(STATUS "OS_PRETTY_NAME = ${OS_PRETTY_NAME}")
message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_CLANG_TIDY = ${CMAKE_CXX_CLANG_TIDY}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO = ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

execute_process(COMMAND git submodule update --init --recursive
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE SUBMODULE_UPDATE
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE REVISION
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE COMMIT
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(COMMAND date "+%Y-%m-%d %H:%M:%S"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE TIMESTAMP
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(VERSION_MAJOR STREQUAL "default")
        execute_process(COMMAND date "+%Y-%m-%d"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE VERSION_MAJOR
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
        )
endif()

foreach(APP ${APP_NAME})
    string(REPLACE "-" "" DEVICE_CLASS "${APP}")
    string(TOLOWER "${DEVICE_CLASS}" DEVICE_CLASS)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version/version.cpp.in
        ${CMAKE_CURRENT_SOURCE_DIR}/application/${APP}/version.cpp
    )
    execute_process(COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/application/${APP}/version.cpp
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE VERSIONCPP
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "cat ${CMAKE_CURRENT_SOURCE_DIR}/application/${APP}/version.cpp: `${VERSIONCPP}`")
endforeach(APP IN APP_NAME)

find_package(yaml-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Lua REQUIRED)

include_directories(
        ${OPENSSL_INCLUDE_DIRS}
        ${Lua_INCLUDE_DIRS}
        ${cpp_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/third/json/
        ${CMAKE_CURRENT_SOURCE_DIR}/third/spdlog/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

add_subdirectory(source)
add_subdirectory(application)
