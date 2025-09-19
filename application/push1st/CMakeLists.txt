application(NAME push1st TYPE EXECUTABLE
    DEPENDENCIES 
    ${yaml-cpp_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${Lua_LIBRARY}
    ${CMAKE_PROJECT_NAME}.http
    ${CMAKE_PROJECT_NAME}.inet 
    ${CMAKE_PROJECT_NAME}.core
    ${CMAKE_PROJECT_NAME}.log
    ${CMAKE_PROJECT_NAME}.channels
)
