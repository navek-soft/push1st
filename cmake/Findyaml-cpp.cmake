# Findyaml-cpp.cmake
# Locate the yaml-cpp library and headers
#
# Defines:
#   yaml-cpp_FOUND
#   yaml-cpp_INCLUDE_DIRS
#   yaml-cpp_LIBRARIES
#
# Also defines:
#   yaml-cpp_INCLUDE_DIR
#   yaml-cpp_LIBRARY
#
# Provides imported target:
#   yaml-cpp::yaml-cpp

set(_YAML_HEADER yaml-cpp/yaml.h)
set(_YAML_NAMES yaml-cpp)

# --- ARM32 toolchain ---
if(PLATFORM_ARM32)
    find_path(yaml-cpp_INCLUDE_DIR
        NAMES ${_YAML_HEADER}
        PATHS
            ${CMAKE_FIND_ROOT_PATH}/include
            ${CMAKE_FIND_ROOT_PATH}/usr/include
    )

    find_library(yaml-cpp_LIBRARY
        NAMES ${_YAML_NAMES}
        PATHS
            ${CMAKE_FIND_ROOT_PATH}/lib
            ${CMAKE_FIND_ROOT_PATH}/usr/lib
    )

# --- AArch64 toolchain ---
elseif(PLATFORM_AARCH64)
    set(TOOLCHAIN_PATH ${CMAKE_FIND_ROOT_PATH})

    find_path(yaml-cpp_INCLUDE_DIR
        NAMES ${_YAML_HEADER}
        PATHS
            ${TOOLCHAIN_PATH}/include
            ${TOOLCHAIN_PATH}/usr/include
            /usr/include
    )

    find_library(yaml-cpp_LIBRARY
        NAMES ${_YAML_NAMES}
        PATHS
            ${TOOLCHAIN_PATH}/lib
            ${TOOLCHAIN_PATH}/usr/lib
            /usr/lib
    )

# --- Host / default ---
else()
    find_path(yaml-cpp_INCLUDE_DIR
        NAMES ${_YAML_HEADER}
        PATHS
            ${CMAKE_CURRENT_LIST_DIR}/../../../yaml-cpp/include
            /usr/local/include
            /usr/include
    )

    find_library(yaml-cpp_LIBRARY
        NAMES ${_YAML_NAMES}
        PATHS
            ${CMAKE_CURRENT_LIST_DIR}/../../../yaml-cpp/lib
            /usr/local/lib
            /usr/lib
            /usr/lib/x86_64-linux-gnu
            /usr/lib/aarch64-linux-gnu
            /usr/lib/arm-linux-gnueabihf
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    yaml-cpp
    REQUIRED_VARS yaml-cpp_LIBRARY yaml-cpp_INCLUDE_DIR
)

if(yaml-cpp_FOUND)
    set(yaml-cpp_INCLUDE_DIRS "${yaml-cpp_INCLUDE_DIR}")
    set(yaml-cpp_LIBRARIES "${yaml-cpp_LIBRARY}")

    if(NOT TARGET yaml-cpp::yaml-cpp)
        add_library(yaml-cpp::yaml-cpp UNKNOWN IMPORTED)
        set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
            IMPORTED_LOCATION "${yaml-cpp_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${yaml-cpp_INCLUDE_DIR}"
        )
    endif()
endif()

unset(_YAML_HEADER)
unset(_YAML_NAMES)
