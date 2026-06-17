include(FetchContent)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-error=invalid-offsetof -Wno-invalid-offsetof -Wno-error=switch)
endif()

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(search_bench
    search_bench.cpp
    ../src/storage.cpp
    ../src/embedder.cpp
    ../src/flat_vector.cpp
)

target_include_directories(search_bench PRIVATE
    ../include
)

target_link_libraries(search_bench PRIVATE
    benchmark::benchmark
)
