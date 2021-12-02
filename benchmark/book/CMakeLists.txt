include(CTest)

option(ENABLE_BENCHMARKS "Enable unit tests" ON)
message(STATUS "Enable benchmarks: ${ENABLE_BENCHMARKS}")

if(ENABLE_BENCHMARKS)
    find_package(benchmark REQUIRED)

    include_directories( ${benchmark_INCLUDE_DIR} )

    add_executable(book_benchmark "book_benchmark.cc")

    set_target_properties( book_benchmark
                           PROPERTIES
                           CXX_STANDARD 20
                           CXX_EXTENSIONS OFF
                           CXX_STANDARD_REQUIRED ON
                           CXX_POSITION_INDEPENDENT_CODE ON )

    target_include_directories( book_benchmark
                                PRIVATE
                                ${CMAKE_SOURCE_DIR}/src/cpp/include )

    target_link_libraries( book_benchmark
                           PRIVATE
                           benchmark::benchmark
                           pthread
                           flatbuf_serialize )

    target_compile_options( book_benchmark PRIVATE "-Werror" )

    add_test( NAME book_benchmark_test_suite
              COMMAND $<TARGET_FILE:book_benchmark> )

endif()
