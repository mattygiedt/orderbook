include(CTest)

option(ENABLE_BENCHMARKS "Enable unit tests" ON)
message(STATUS "Enable benchmarks: ${ENABLE_BENCHMARKS}")

if(ENABLE_BENCHMARKS)
    find_package(benchmark REQUIRED)

    include_directories( ${benchmark_INCLUDE_DIR} )

    add_executable(container_benchmark "container_benchmark.cc")

    set_target_properties( container_benchmark
                           PROPERTIES
                           CXX_STANDARD 20
                           CXX_EXTENSIONS OFF
                           CXX_STANDARD_REQUIRED ON
                           CXX_POSITION_INDEPENDENT_CODE ON )

    target_include_directories( container_benchmark
                                PRIVATE
                                ${CMAKE_SOURCE_DIR}/src/cpp/include )

    target_link_libraries( container_benchmark
                           PRIVATE
                           benchmark::benchmark
                           gtest
                           pthread
                           flatbuf_serialize )

    target_compile_options( container_benchmark PRIVATE "-Werror" )

    add_test( NAME container_benchmark_test_suite
              COMMAND $<TARGET_FILE:container_benchmark> )

endif()
