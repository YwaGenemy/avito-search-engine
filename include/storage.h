#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "ad.h"

class DocumentStorage {
   public:
    IdType Add(Ad ad);

    bool Remove(IdType id) { return ads_.erase(id) > 0; }

    std::optional<Ad> Get(IdType id) const;

    const std::unordered_map<IdType, Ad>& All() const { return ads_; }

    size_t Size() const { return ads_.size(); }

    bool Empty() const { return ads_.empty(); }

    void Clear() {
        ads_.clear();
        next_id_ = 1;
    }

   private:
    IdType next_id_ = 1;
    std::unordered_map<IdType, Ad> ads_;
};