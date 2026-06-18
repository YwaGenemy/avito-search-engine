include(FetchContent)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-error=invalid-offsetof -Wno-invalid-offsetof -Wno-error=switch)
endif()

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(search_bench
    search_bench.cpp
    dataset_loader.cpp
    ../src/storage.cpp
    ../src/bm25_index.cpp
    ../src/embedder.cpp
    ../src/flat_vector.cpp
)

target_include_directories(search_bench PRIVATE
    ../include
)

target_link_libraries(search_bench PRIVATE
    benchmark::benchmark
    nlohmann_json::nlohmann_json
)
