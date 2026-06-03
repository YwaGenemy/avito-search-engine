#pragma once

#include <cstddef>

struct IndexStats {
    std::size_t documents_count = 0;
    std::size_t categories_count = 0;
    std::size_t embedding_dimension = 0;
    std::size_t memory_bytes = 0;
};
