#pragma once

#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "ad.h"

class DocumentStorage {
   public:
    IdType Add(Ad ad);

    bool Remove(IdType id);

    std::optional<Ad> Get(IdType id) const;

    std::unordered_map<IdType, Ad> All() const;

    size_t Size() const;

    bool Empty() const;

    void Clear();

   private:
    mutable std::shared_mutex mutex_;
    IdType next_id_ = 1;
    std::unordered_map<IdType, Ad> ads_;
};