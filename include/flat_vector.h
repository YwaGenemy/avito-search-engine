#pragma once

#include "ad.h"
#include "index_stats.h"
#include "index.h"

#include "search_options.h"
#include "search_result.h"

#include "embedder.h"

#include<bits/stdc++.h> // ВРЕМЕННО.

class FlatVectorIndex : public Index{
public:
    explicit FlatVectorIndex(size_t emb_dimension = 128);
    IndexType Type() const noexcept override;
    void Add(const Ad& ad) override;

    std::vector<SearchResult> Search(const std::string& query, const SearchOptions& options)const override;
    std::optional<Ad> Get(int ad_id)const override;
    IndexStats Stats() const override;
    size_t Size() const override;
    void Clear() override;

private:

    std::vector<Ad> ads_;
    std::vector<std::vector<float>> embeddings_;
    std::unordered_map<int, size_t> id_to_pos_;
    Embedder embedder_;

    // mutable std::shared_mutex mutex_;

};



