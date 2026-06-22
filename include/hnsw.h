#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>
#include <set>
#include <shared_mutex>

#include "index.h"
#include "embedder.h"

class HnswIndex final : public Index {
public:
    HnswIndex(DocumentStorage& storage, size_t emb_dimension, size_t M = 16, size_t ef_construction = 200);
    ~HnswIndex() override = default;

    IndexType Type() const noexcept override;

    void Add(const Ad& ad) override;

    void Remove(IdType id) override;

    std::vector<SearchResult> Search(const std::string& query, const SearchOptions& options) const override;

    std::optional<Ad> Get(IdType ad_id) const override;

    IndexStats Stats() const override;

    size_t Size() const override;

    void Clear() override;

private:
    struct Node {
        IdType id;
        std::vector<float> embedding;
        int level;
        std::vector<std::set<IdType>> neighbors;

        Node(IdType id_, std::vector<float> emb_, int level_) : id(id_), embedding(std::move(emb_)), level(level_), neighbors(level_ + 1) {}
    };

    double CosineDistance(const std::vector<float>& a, const std::vector<float>& b) const;

    std::vector<IdType> SearchLayer(IdType entry_point, const std::vector<float>& query_emb, int layer, size_t ef) const;

    std::vector<IdType> KnnSearch(const std::vector<float>& query_emb, size_t K, const std::optional<std::string>& category = std::nullopt) const;

    int GetRandomLevel() const;
    void ConnectNewNode(IdType new_id, const std::vector<IdType>& nearest, int level);

    mutable std::shared_mutex mtx_;
    Embedder embedder_;

    const size_t M_;
    const size_t ef_construction_;
    const size_t max_level_;

    std::unordered_map<IdType, std::unique_ptr<Node>> nodes_;

    IdType entry_point_ = 0;
    int max_level_global_ = -1;
    size_t doc_count_ = 0;

    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<double> dist_;

    static constexpr double ML_ = 1.0 / std::log(2.0);
};
