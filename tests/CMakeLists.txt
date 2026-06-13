include(FetchContent)

find_package(GTest QUIET)

if(NOT GTest_FOUND)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(search_engine_tests
    embedder_test.cpp
    flat_vector_test.cpp
    ../src/embedder.cpp
    ../src/flat_vector.cpp
    ../src/storage.cpp
)

target_include_directories(search_engine_tests PRIVATE
    ../include
)

target_link_libraries(search_engine_tests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(search_engine_tests)
