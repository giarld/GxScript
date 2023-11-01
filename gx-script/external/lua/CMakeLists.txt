cmake_minimum_required(VERSION 3.20)

project(LUA)

file(GLOB_RECURSE LUA_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_library(lua
        ${LUA_SRCS}
        )

if (BUILD_SHARED_LIBS AND CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_compile_definitions(lua PUBLIC LUA_BUILD_AS_DLL)
endif ()

target_include_directories(lua PUBLIC src/)

# static
add_library(lua-static STATIC
        ${LUA_SRCS}
        )

target_include_directories(lua-static PUBLIC src/)
