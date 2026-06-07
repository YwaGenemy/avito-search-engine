#pragma once

#include <cstdint>
#include <string>

using IdType = uint64_t;

struct Ad {
    std::string title;
    std::string description;
    std::string category;

    std::string Text() const { return title + ' ' + description; }

    void SetID(IdType id) { id_ = id; }

    uint64_t GetID() { return id_; }

   private:
    IdType id_;
};
