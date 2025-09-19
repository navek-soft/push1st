macro(apply_source_groups USER_FILES DIRECTORY GROUP_PREFIX)
    foreach(USER_FILE ${${USER_FILES}})
        file(RELATIVE_PATH SRC_GROUP ${DIRECTORY} ${USER_FILE})
        set(SRC_GROUP "${GROUP_PREFIX}/${SRC_GROUP}")
        string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRC_GROUP ${SRC_GROUP})
        string(REPLACE / \\ SRC_GROUP ${SRC_GROUP})
        source_group("${SRC_GROUP}" FILES ${USER_FILE})
    endforeach()
endmacro()

function(directories_files dir files)
    file(GLOB_RECURSE HPP_FILES "${dir}/*.h*")
    file(GLOB_RECURSE SRC_CPP_FILES "${dir}/*.*pp")
    file(GLOB_RECURSE SRC_C_FILES "${dir}/*.c")
    file(GLOB_RECURSE SRC_C_FILES "${dir}/*.cc")
    set(${files} ${HPP_FILES} ${SRC_CPP_FILES} ${SRC_C_FILES} PARENT_SCOPE)
    apply_source_groups(HPP_FILES ${CMAKE_CURRENT_SOURCE_DIR} "Headers")
    apply_source_groups(SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR} "Sources")
endfunction()

macro(application)
    set(options)
    set(oneValueArgs NAME TYPE OUTPUT_DIRECTORY)
    set(multiValueArgs DEPENDENCIES INCLUDE_DIRECTORIES)

    cmake_parse_arguments(APPLICATION "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT DEFINED APPLICATION_TYPE)
        set(APPLICATION_TYPE EXECUTABLE)
    endif()

    if(NOT DEFINED APPLICATION_OUTPUT_DIRECTORY)
        set(APPLICATION_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    endif()

    directories_files(${CMAKE_CURRENT_SOURCE_DIR} FILES)

    if("${APPLICATION_TYPE}" STREQUAL "SHARED" OR "${APPLICATION_TYPE}" STREQUAL "shared")
        message(STATUS "application ${APPLICATION_NAME} is building as a shared library")
        add_library(${APPLICATION_NAME} SHARED ${FILES})

        set_target_properties(${APPLICATION_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${APPLICATION_OUTPUT_DIRECTORY}"
            DEBUG_POSTFIX ""
        )
    elseif("${APPLICATION_TYPE}" STREQUAL "STATIC" OR "${APPLICATION_TYPE}" STREQUAL "static")
        message(STATUS "application ${APPLICATION_NAME} is building as an static library")
        add_library(${APPLICATION_NAME} STATIC ${FILES})

        set_target_properties(${APPLICATION_NAME} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${APPLICATION_OUTPUT_DIRECTORY}"
            DEBUG_POSTFIX ""
        )
    else()
        add_executable(${APPLICATION_NAME} ${FILES})
        set_target_properties(${APPLICATION_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${APPLICATION_OUTPUT_DIRECTORY}"
            DEBUG_POSTFIX ""
        )

        if(${BUILD_TYPE} STREQUAL "relwithdebinfo")
            if (PLATFORM_AARCH64 OR PLATFORM_ARM32)
                find_program(OBJCOPY NAMES ${CMAKE_SYSTEM_NAME}-objcopy  REQUIRED)
            else()
                find_program(OBJCOPY NAMES objcopy REQUIRED)
            endif()
            message(STATUS "application ${APPLICATION_NAME} is building as an executable file with debug info ${APPLICATION_OUTPUT_DIRECTORY}/${APPLICATION_NAME}.debuginfo")
            add_custom_command(
                TARGET ${APPLICATION_NAME}
                POST_BUILD
                COMMAND ${OBJCOPY} --only-keep-debug ${APPLICATION_OUTPUT_DIRECTORY}/${APPLICATION_NAME} ${APPLICATION_OUTPUT_DIRECTORY}/${APPLICATION_NAME}.debuginfo
                COMMAND ${OBJCOPY} --strip-debug --strip-unneeded ${APPLICATION_OUTPUT_DIRECTORY}/${APPLICATION_NAME}
                COMMENT "Generating and attaching debug symbols to stripped binary"
            )
        else()
            message(STATUS "application ${APPLICATION_NAME} is building as an executable file")
        endif()
    endif()

    if(DEFINED APPLICATION_DEPENDENCIES)
        message(STATUS "-- dependencies ${APPLICATION_DEPENDENCIES}")
        target_link_libraries(${APPLICATION_NAME} PRIVATE ${APPLICATION_DEPENDENCIES})
    endif()

    if(DEFINED APPLICATION_INCLUDE_DIRECTORIES)
        message(STATUS "-- include directories ${APPLICATION_INCLUDE_DIRECTORIES}")
        target_include_directories(${APPLICATION_NAME} PRIVATE ${APPLICATION_INCLUDE_DIRECTORIES})
    endif()
endmacro()

macro(module)
    set(options)
    set(oneValueArgs NAME TYPE OUTPUT_DIRECTORY)
    set(multiValueArgs DEPENDENCIES INCLUDE_DIRECTORIES)

    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT DEFINED MODULE_TYPE)
        set(MODULE_TYPE object)
    endif()

    if(NOT DEFINED MODULE_OUTPUT_DIRECTORY)
        set(MODULE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/module)
    endif()

    set(ORIGIN_MODULE_NAME "${MODULE_NAME}")
    set(MODULE_NAME "${PROJECT_NAME}.${MODULE_NAME}")

    directories_files(${CMAKE_CURRENT_SOURCE_DIR} FILES)

    if("${MODULE_TYPE}" STREQUAL "SHARED" OR "${MODULE_TYPE}" STREQUAL "shared")
        message(STATUS "module ${MODULE_NAME} is building as a shared library")
        add_library(${MODULE_NAME} SHARED ${FILES})

        set_target_properties(${MODULE_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${MODULE_OUTPUT_DIRECTORY}"
            DEBUG_POSTFIX ""
        )
    elseif("${MODULE_TYPE}" STREQUAL "OBJECT" OR "${MODULE_TYPE}" STREQUAL "object")
        message(STATUS "module ${MODULE_NAME} is building as a object library")
        add_library(${MODULE_NAME} OBJECT ${FILES})

        set_target_properties(${MODULE_NAME} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${MODULE_OUTPUT_DIRECTORY}"
            DEBUG_POSTFIX ""
        )
    else()
        message(STATUS "module ${MODULE_NAME} is building as a static library")
        add_library(${MODULE_NAME} STATIC ${FILES})

        set_target_properties(${MODULE_NAME} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${MODULE_OUTPUT_DIRECTORY}"
            DEBUG_POSTFIX ""
        )
    endif()

    if(DEFINED MODULE_DEPENDENCIES)
        message(STATUS "-- dependencies ${MODULE_DEPENDENCIES}")
        target_link_libraries(${MODULE_NAME} ${MODULE_DEPENDENCIES})
    endif()

    if(DEFINED MODULE_INCLUDE_DIRECTORIES)
        message(STATUS "-- include directories ${MODULE_INCLUDE_DIRECTORIES}")
        target_include_directories(${MODULE_NAME} PRIVATE ${MODULE_INCLUDE_DIRECTORIES})
    endif()

    target_compile_definitions(${MODULE_NAME} PRIVATE __MODULE_NAME__="${ORIGIN_MODULE_NAME}")
endmacro()

macro(test)
    set(options)
    set(oneValueArgs NAME TYPE OUTPUT_DIRECTORY)
    set(multiValueArgs DEPENDENCIES INCLUDE_DIRECTORIES)

    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT DEFINED TEST_OUTPUT_DIRECTORY)
        set(TEST_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)
    endif()

    directories_files(${CMAKE_CURRENT_SOURCE_DIR} FILES)

    find_package(Catch2 CONFIG REQUIRED)

    message(STATUS "test ${TEST_NAME}.test is building as an executable file")
    add_executable(${TEST_NAME}.test ${FILES})
    set_target_properties(${TEST_NAME}.test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${TEST_OUTPUT_DIRECTORY}"
        DEBUG_POSTFIX ""
    )

    if(DEFINED TEST_DEPENDENCIES)
        message(STATUS "-- dependencies ${APPLICATION_DEPENDENCIES}")
        target_link_libraries(${TEST_NAME}.test PRIVATE Catch2::Catch2WithMain ${TEST_DEPENDENCIES})
    endif()

    if(DEFINED TEST_INCLUDE_DIRECTORIES)
        message(STATUS "-- include directories ${TEST_INCLUDE_DIRECTORIES}")
        target_include_directories(${TEST_NAME}.test PRIVATE ${TEST_INCLUDE_DIRECTORIES})
    endif()

    include(CTest)
    include(Catch)
    catch_discover_tests(${TEST_NAME}.test)
endmacro()
