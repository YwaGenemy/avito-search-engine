#include "storage.h"

#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "ad.h"

bool DocumentStorage::Remove(IdType id) {
    std::unique_lock lock(mutex_);
    return ads_.erase(id) > 0;
}

IdType DocumentStorage::Add(Ad ad) {
    std::unique_lock lock(mutex_);

    const IdType id = next_id_++;
    ad.SetID(id);
    ads_[id] = std::move(ad);
    return id;
}

std::optional<Ad> DocumentStorage::Get(IdType id) const {
    std::shared_lock lock(mutex_);

    const auto it = ads_.find(id);
    if (it == ads_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::unordered_map<IdType, Ad> DocumentStorage::All() const {
    std::shared_lock lock(mutex_);
    return std::move(ads_);
}

size_t DocumentStorage::Size() const {
    std::shared_lock lock(mutex_);
    return ads_.size();
}

bool DocumentStorage::Empty() const {
    std::shared_lock lock(mutex_);
    return ads_.empty();
}

void DocumentStorage::Clear() {
    std::unique_lock lock(mutex_);

    ads_.clear();
    next_id_ = 1;
}
