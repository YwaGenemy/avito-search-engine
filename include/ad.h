#pragma once

#include <cstdint>
#include <string>

using IdType = uint64_t;

struct Ad {
    Ad() = default;

    Ad(std::string title, std::string description, std::string category)
        : title(std::move(title)),
          description(std::move(description)),
          category(std::move(category)) {}

    std::string Text() const { return title + ' ' + description; }

    IdType GetID() const { return id_; }

    std::string title;
    std::string description;
    std::string category;

   private:
    friend class DocumentStorage;

    void SetID(IdType id) { id_ = id; }

    IdType id_ = 0;
};
