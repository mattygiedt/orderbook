cmake_minimum_required(VERSION 3.1...3.16.8 FATAL_ERROR)

find_package(spdlog REQUIRED)
find_package(cppzmq REQUIRED)

add_executable( orderbook "./src/orderbook.cc" )

set_target_properties( orderbook
                       PROPERTIES
                       CXX_STANDARD 20
                       CXX_EXTENSIONS OFF
                       CXX_STANDARD_REQUIRED ON
                       CXX_POSITION_INDEPENDENT_CODE ON )

target_include_directories( orderbook
                            PUBLIC
                            ${CMAKE_CURRENT_SOURCE_DIR}/include )

target_link_libraries( orderbook
                       PUBLIC
                       flatbuf_serialize
                       spdlog::spdlog
                       cppzmq
                       tcmalloc )

add_executable( client "./src/client.cc" )

set_target_properties( client
                       PROPERTIES
                       CXX_STANDARD 20
                       CXX_EXTENSIONS OFF
                       CXX_STANDARD_REQUIRED ON
                       CXX_POSITION_INDEPENDENT_CODE ON )

target_include_directories( client
                            PUBLIC
                            ${CMAKE_CURRENT_SOURCE_DIR}/include )

target_link_libraries( client
                       PUBLIC
                       flatbuf_serialize
                       spdlog::spdlog
                       cppzmq
                       tcmalloc )

add_executable( gateway "./src/gateway.cc" )

set_target_properties( gateway
                       PROPERTIES
                       CXX_STANDARD 20
                       CXX_EXTENSIONS OFF
                       CXX_STANDARD_REQUIRED ON
                       CXX_POSITION_INDEPENDENT_CODE ON )

target_include_directories( gateway
                            PUBLIC
                            ${CMAKE_CURRENT_SOURCE_DIR}/include )

target_link_libraries( gateway
                       PUBLIC
                       flatbuf_serialize
                       spdlog::spdlog
                       quickfix
                       cppzmq
                       tcmalloc )
