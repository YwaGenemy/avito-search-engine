#pragma once

#include <cstdint>
#include <string>

struct Ad {
    int id = 0;

    std::string title;
    std::string description;
    std::string category;

    size_t TextSize() const { return title.size() + description.size(); }

    std::string Text() const { return title + ' ' + description; }
};
