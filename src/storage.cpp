#include "storage.h"

#include <optional>
#include <unordered_map>
#include <vector>

#include "ad.h"

IdType DocumentStorage::Add(Ad ad) {
    const IdType id = next_id_++;
    ad.SetID(id);
    ads_[id] = std::move(ad);
    return id;
}

std::optional<Ad> DocumentStorage::Get(IdType id) const {
    const auto it = ads_.find(id);
    if (it == ads_.end()) {
        return std::nullopt;
    }

    return it->second;
}
