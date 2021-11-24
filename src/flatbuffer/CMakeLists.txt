cmake_minimum_required(VERSION 3.1...3.16.8 FATAL_ERROR)

find_package(Flatbuffers REQUIRED)

file(GLOB flatc_SRC "*.fbs")

foreach(FILE ${flatc_SRC})
    message(STATUS "generating flatbuffer file: ${FILE}")
    execute_process(COMMAND flatc --cpp --cpp-std c++17 -o ${CMAKE_CURRENT_BINARY_DIR}/include/orderbook/serialize/ ${FILE}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

message(STATUS "creating flatbuf_serialize library: ${CMAKE_CURRENT_BINARY_DIR}/include")
add_library(flatbuf_serialize INTERFACE)
target_include_directories(flatbuf_serialize INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/include)
