module(NAME core TYPE STATIC
    DEPENDENCIES 
    ${yaml-cpp_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${Lua_LIBRARY}
    ${CMAKE_PROJECT_NAME}.http     
    ${CMAKE_PROJECT_NAME}.inet
)
    