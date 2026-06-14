find_package(GTest REQUIRED)

add_executable(bm25_index_tests
    bm25_index_tests.cpp
)

target_link_libraries(bm25_index_tests
    PRIVATE
        search_engine_core
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(bm25_index_tests)
