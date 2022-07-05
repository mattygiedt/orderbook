cmake_minimum_required(VERSION 3.1...3.16.8 FATAL_ERROR)

project(engine VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### Verbose builds ###
set(CMAKE_VERBOSE_MAKEFILE OFF)

### Build Examples Flag ###
set(BUILD_EXAMPLES OFF)

# Output compiler details
if(CMAKE_CXX_COMPILER_LOADED)
    message(STATUS "Compiler Loaded: ${CMAKE_CXX_COMPILER_LOADED}")
    message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
    message(STATUS "C++ compiler ID: ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "C++ compiler version is: ${CMAKE_CXX_COMPILER_VERSION}")
else()
    message(FATAL_ERROR "C++ compiler ID is: ${CMAKE_CXX_COMPILER_ID_GXX}")
endif()

### Setup configurations ###

# Set default configuration mode to debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# add_compile_options( -Wall -Wextra )
add_compile_options( -Wall
                     -Wextra
                     -pedantic
                     -Werror )

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#   Example setting the release type:
#
#       cmake -DCMAKE_BUILD_TYPE=Release ..
#

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(BUILD_EXAMPLES ON)
    list(APPEND CMAKE_CXX_FLAGS_DEBUG
        "-ggdb3" # add extra debugging info
    )
    string(REPLACE ";" " " CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    message(STATUS "C++ flags, base Debug configuration: ${CMAKE_CXX_FLAGS_DEBUG}")
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message(STATUS "C++ flags, base Release configuration: ${CMAKE_CXX_FLAGS_RELEASE}")
elseif(${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
    message(STATUS "C++ flags, base Release configuration with Debug info: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
elseif(${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel")
    message(STATUS "C++ flags, base minimal Release configuration: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
else()
     message(FATAL_ERROR "Invalid build configuration: ${CMAKE_BUILD_TYPE}")
endif()


### Setup linting ###

find_program( CLANG_TIDY_EXE NAMES "clang-tidy" DOC "Path to clang-tidy executable" )
if(NOT CLANG_TIDY_EXE)
    message(SEND_ERROR "Linter clang-tidy not found.")
else()
    message(STATUS "Linter clang-tidy found: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY clang-tidy
        -header-filter=*.h -checks=-*,modernize-*,diagnostic-*,analyzer-*,readability-*,performance-*)
endif()

# Subdirectories
add_subdirectory(src)

# Testing
enable_testing()
add_subdirectory(test)
add_subdirectory(benchmark)
