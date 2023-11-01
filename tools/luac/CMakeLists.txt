cmake_minimum_required(VERSION 3.20)

project(LUAC)

set(TARGET_NAME luac)

add_executable(${TARGET_NAME} src/luac.cpp)

target_link_libraries(${TARGET_NAME} gany-core gx lua-static)
