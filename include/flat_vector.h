#pragma once

#include "ad.h"
#include "index_stats.h"
#include "index.h"

#include "search_options.h"
#include "search_result.h"

#include "embedder.h"

#include "storage.h"

#include <cstddef>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

class FlatVectorIndex : public Index{
public:
    explicit FlatVectorIndex(DocumentStorage& storage, size_t emb_dimension = 128);
    IndexType Type() const noexcept override;

    void Add(const Ad& ad) override;
    void Remove(IdType id) override;

    std::vector<SearchResult> Search(const std::string& query, const SearchOptions& options)const override;
    std::optional<Ad> Get(IdType ad_id)const override;
    IndexStats Stats() const override;
    size_t Size() const override;
    void Clear() override;

private:
    // std::unoredered_map<IdType, Ad> storage_; <- DocumentStorage
    std::unordered_map<IdType, std::vector<float>> embeddings_;
    Embedder embedder_;
    mutable std::shared_mutex mtx_;

};


