cmake_minimum_required(VERSION 3.20)
project(GxScript)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT BUILD_SHARED_LIBS)
    option(BUILD_SHARED_LIBS "build shared libs" ON)
endif ()

if (NOT CMAKE_SIZEOF_VOID_P)
    if (CMAKE_CL_64)
        set(CMAKE_SIZEOF_VOID_P 8)
    else ()
        set(CMAKE_SIZEOF_VOID_P 4)
    endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "CMAKE_BUILD_TYPE must be a valid value")
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
    SET(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
endif ()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif ()


if (NOT T_EXECUTABLE_OUTPUT_PATH)
    set(T_EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
endif ()

if (NOT T_LIBRARY_OUTPUT_PATH)
    set(T_LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${T_EXECUTABLE_OUTPUT_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${T_EXECUTABLE_OUTPUT_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${T_EXECUTABLE_OUTPUT_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${T_EXECUTABLE_OUTPUT_PATH})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${T_LIBRARY_OUTPUT_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${T_LIBRARY_OUTPUT_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${T_LIBRARY_OUTPUT_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${T_LIBRARY_OUTPUT_PATH})

if (NOT WIN32)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${T_LIBRARY_OUTPUT_PATH})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${T_LIBRARY_OUTPUT_PATH})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${T_LIBRARY_OUTPUT_PATH})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${T_LIBRARY_OUTPUT_PATH})
endif ()


option(ENABLE_BUILD_TOOLS "Enable Tools Build." ON)
option(ENABLE_BUILD_TEST "Enable Test Build." ON)

if (NOT GX_LIBS_INSTALL_DIR)
    set(GX_LIBS_INSTALL_DIR ${CMAKE_BINARY_DIR}/dev)
endif ()

include(cmake/GetGitDep.cmake)

GetGitDependency(git@github.com:giarld/GxAny.git GxAny main)
GetGitDependency(git@github.com:giarld/GxLib.git GxLib main)

add_subdirectory(deps/GxAny/gany-interface)
add_subdirectory(deps/GxAny/gany-core)
add_subdirectory(deps/GxLib/gx)
add_subdirectory(gx-script)

if (ENABLE_BUILD_TOOLS)
    add_subdirectory(tools/luac)
endif ()

if (ENABLE_BUILD_TEST)
    GetGitDependency(git@github.com:google/googletest.git gtest v1.14.0)
    add_subdirectory(deps/gtest)

    add_subdirectory(test)
endif ()
