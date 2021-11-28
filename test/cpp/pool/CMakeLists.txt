include(CTest)

option(ENABLE_UNIT_TESTS "Enable unit tests" ON)
message(STATUS "Enable testing: ${ENABLE_UNIT_TESTS}")

if(ENABLE_UNIT_TESTS)
    find_package(GTest REQUIRED)

    message(STATUS "creating tests: ${CMAKE_CURRENT_SOURCE_DIR}")

    include_directories( ${GTest_INCLUDE_DIR} )

    add_executable(pool_tests "pool_tests.cc")

    set_target_properties( pool_tests
                           PROPERTIES
                           CXX_STANDARD 20
                           CXX_EXTENSIONS OFF
                           CXX_STANDARD_REQUIRED ON
                           CXX_POSITION_INDEPENDENT_CODE ON )

    target_include_directories( pool_tests
                                PRIVATE
                                /workspaces/orderbook/src/cpp/include )

    target_link_libraries( pool_tests
                           PRIVATE
                           gtest
                           gtest_main
                           pthread
                           flatbuf_serialize )

    target_compile_options( pool_tests PRIVATE "-Werror" )

    add_test( NAME pool_test_suite
              COMMAND $<TARGET_FILE:pool_tests> )

endif()
