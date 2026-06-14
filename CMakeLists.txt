cmake_minimum_required(VERSION 3.20)

project(avito_search_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(search_engine_core
    src/storage.cpp
    # src/bm25_index.cpp
    src/bm25_index.cpp
    #flat_vector
    src/embedder.cpp 
    src/flat_vector.cpp
)

target_include_directories(search_engine_core PUBLIC
    include
)

add_executable(search_engine
    src/main.cpp
    src/cli.cpp
)

target_link_libraries(search_engine PRIVATE
    search_engine_core)

enable_testing()

add_subdirectory(tests)
add_subdirectory(benchmarks)
