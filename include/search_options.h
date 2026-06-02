#pragma once

#include <cstddef>
#include <optional>
#include <string>

struct SearchOptions {
    std::optional<std::string> category;
    std::size_t top_k = 10;
    std::size_t offset = 0;
};
