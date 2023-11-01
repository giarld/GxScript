cmake_minimum_required(VERSION 3.20)

add_executable(TestGxScript
        src/test_main.cpp
        src/test_script.cpp
)

target_link_libraries(TestGxScript gtest gany-core gx gx-script)
