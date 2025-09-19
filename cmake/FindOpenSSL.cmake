# FindOpenSSL.cmake
# This module defines the following variables:
# OpenSSL_FOUND        - Set to true if OpenSSL is found
# OpenSSL_INCLUDE_DIR  - The directory where the OpenSSL headers are located
# OpenSSL_LIBRARIES    - The libraries to link against

if(PLATFORM_ARM32)
  set(TOOLCHAIN_PATH ${CMAKE_FIND_ROOT_PATH})
  set(SEARCH_PATHS
    ${TOOLCHAIN_PATH}/include
    ${TOOLCHAIN_PATH}/lib
  )
  set(SEARCH_LIBSSL libssl.a)
  set(SEARCH_LIBCRYPTO libcrypto.a)
elseif(PLATFORM_AARCH64)
  set(TOOLCHAIN_PATH ${CMAKE_FIND_ROOT_PATH})
  set(SEARCH_PATHS
    /usr/include
    ${TOOLCHAIN_PATH}
  )
  set(SEARCH_LIBSSL libssl.a)
  set(SEARCH_LIBCRYPTO libcrypto.a)
else()
  set(SEARCH_PATHS
    /usr/local/include
    /usr/include
    /usr/local/lib
    /usr/lib
  )
  set(SEARCH_LIBSSL libssl.so)
  set(SEARCH_LIBCRYPTO libcrypto.so)
endif()

find_path(OpenSSL_INCLUDE_DIR
  NAMES openssl/ssl.h
  PATHS ${SEARCH_PATHS}
)

find_library(OpenSSL_SSL_LIBRARY
  NAMES ${SEARCH_LIBSSL}
  PATHS ${SEARCH_PATHS}
)

find_library(OpenSSL_CRYPTO_LIBRARY
  NAMES ${SEARCH_LIBCRYPTO}
  PATHS ${SEARCH_PATHS}
)

# Combine the libraries into a list
set(OpenSSL_LIBRARIES ${OpenSSL_SSL_LIBRARY} ${OpenSSL_CRYPTO_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenSSL DEFAULT_MSG OpenSSL_INCLUDE_DIR OpenSSL_LIBRARIES)

if(OpenSSL_FOUND)
  set(OPENSSL_INCLUDE_DIRS ${OpenSSL_INCLUDE_DIR})
  set(OPENSSL_LIBRARIES ${OpenSSL_LIBRARIES})
endif()
