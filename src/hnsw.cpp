#include "hnsw.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <random>
#include <chrono>
#include <limits>
#include <mutex>
#include <unordered_set>

namespace {
std::size_t CalcCategories(const DocumentStorage& storage) {
    std::unordered_set<std::string> us_cat;
    for (auto& [id, ad] : storage.All()) {
        us_cat.insert(ad.category);
    }
    return us_cat.size();
}

} // namespace

HnswIndex::HnswIndex(DocumentStorage& storage, size_t emb_dimension, size_t M, size_t ef_construction)
    : Index(storage)
    , embedder_(emb_dimension)
    , M_(M)
    , ef_construction_(ef_construction)
    , max_level_(0)
    , rng_(std::chrono::steady_clock::now().time_since_epoch().count())
    , dist_(0.0, 1.0) {}

IndexType HnswIndex::Type() const noexcept {
    return IndexType::Hnsw;
}

double HnswIndex::CosineDistance(const std::vector<float>& a, const std::vector<float>& b) const {
    double dot = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;

    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    if (norm_a == 0.0 || norm_b == 0.0) {
        return 1.0;
    }

    return 1.0 - dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

int HnswIndex::GetRandomLevel() const {
    double r = dist_(rng_);
    double level = -std::log(r) * (1.0 / std::log(static_cast<double>(M_)));
    return std::min(static_cast<int>(level), 20);
}

std::vector<IdType> HnswIndex::SearchLayer(IdType entry_point, const std::vector<float>& query_emb, int layer, size_t ef) const {
    struct NodeDist {
        IdType id;
        double dist;

        bool operator<(const NodeDist& other) const {
            return dist < other.dist;
        }

        bool operator>(const NodeDist& other) const {
            return dist > other.dist;
        }
    };

    std::priority_queue<NodeDist, std::vector<NodeDist>, std::greater<NodeDist>> candidates;
    std::priority_queue<NodeDist> results;

    auto it = nodes_.find(entry_point);
    if (it == nodes_.end()) {
        return {};
    }

    const Node* entry_node = it->second.get();
    double entry_dist = CosineDistance(query_emb, entry_node->embedding);

    candidates.push({entry_point, entry_dist});
    results.push({entry_point, entry_dist});

    std::unordered_set<IdType> visited;
    visited.insert(entry_point);

    while (!candidates.empty()) {
        NodeDist current = candidates.top();
        candidates.pop();

        if (current.dist > results.top().dist) {
            break;
        }

        const auto node_it = nodes_.find(current.id);
        if (node_it == nodes_.end()) {
            continue;
        }

        const Node* node = node_it->second.get();

        if (static_cast<size_t>(layer) < node->neighbors.size()) {
            for (IdType neighbor_id : node->neighbors[layer]) {
                if (visited.find(neighbor_id) != visited.end()) {
                    continue;
                }
                visited.insert(neighbor_id);

                const auto neighbor_it = nodes_.find(neighbor_id);
                if (neighbor_it == nodes_.end()) {
                    continue;
                }

                double dist = CosineDistance(query_emb, neighbor_it->second->embedding);

                if (dist < results.top().dist || results.size() < ef) {
                    candidates.push({neighbor_id, dist});
                    results.push({neighbor_id, dist});

                    if (results.size() > ef) {
                        results.pop();
                    }
                }
            }
        }
    }

    std::vector<IdType> result_ids;
    result_ids.reserve(results.size());

    while (!results.empty()) {
        result_ids.push_back(results.top().id);
        results.pop();
    }

    std::reverse(result_ids.begin(), result_ids.end());

    return result_ids;
}

std::vector<IdType> HnswIndex::KnnSearch(const std::vector<float>& query_emb, size_t K, const std::optional<std::string>& category) const {
    std::shared_lock sl(mtx_);

    if (nodes_.empty() || K == 0) {
        return {};
    }

    if (nodes_.size() == 1) {
        for (const auto& [id, node] : nodes_) {
            return {id};
        }
    }

    IdType current_point = entry_point_;
    if (nodes_.find(current_point) == nodes_.end()) {
        for (const auto& [id, node] : nodes_) {
            current_point = id;
            break;
        }
    }

    int current_level = max_level_global_;

    for (int level = current_level; level > 0; --level) {
        std::vector<IdType> nearest = SearchLayer(current_point, query_emb, level, 1);
        if (!nearest.empty()) {
            current_point = nearest[0];
        }
    }

    size_t ef = std::max(ef_construction_, K);
    std::vector<IdType> nearest = SearchLayer(current_point, query_emb, 0, ef);

    std::vector<IdType> filtered;
    filtered.reserve(nearest.size());

    for (IdType id : nearest) {
        if (category.has_value()) {
            auto ad_opt = storage_.Get(id);
            if (!ad_opt.has_value() || ad_opt->category != *category) {
                continue;
            }
        }
        filtered.push_back(id);
        if (filtered.size() >= K) {
            break;
        }
    }

    return filtered;
}

void HnswIndex::Add(const Ad& ad) {
    std::unique_lock ul(mtx_);

    std::vector<float> embedding = embedder_.Embed(ad.Text());

    IdType new_id = storage_.Add(ad);

    int level = GetRandomLevel();

    auto new_node = std::make_unique<Node>(new_id, std::move(embedding), level);

    if (nodes_.empty()) {
        entry_point_ = new_id;
        max_level_global_ = level;
        nodes_[new_id] = std::move(new_node);
        doc_count_++;
        return;
    }

    IdType current_point = entry_point_;

    for (int l = max_level_global_; l > level; --l) {
        std::vector<IdType> nearest = SearchLayer(current_point, new_node->embedding, l, 1);

        if (!nearest.empty()) {
            current_point = nearest[0];
        }
    }

    for (int l = std::min(level, max_level_global_); l >= 0; --l) {
        size_t ef = std::max(ef_construction_, M_ * 2);
        std::vector<IdType> nearest = SearchLayer(current_point, new_node->embedding, l, ef);

        for (IdType neighbor_id : nearest) {
            if (neighbor_id == new_id) continue;

            if (static_cast<size_t>(l) < new_node->neighbors.size()) {
                new_node->neighbors[l].insert(neighbor_id);
            }

            auto neighbor_it = nodes_.find(neighbor_id);
            if (neighbor_it != nodes_.end()) {
                Node* neighbor = neighbor_it->second.get();
                if (static_cast<size_t>(l) < neighbor->neighbors.size()) {
                    neighbor->neighbors[l].insert(new_id);
                }

                if (neighbor->neighbors[l].size() > M_) {
                    std::vector<std::pair<IdType, double>> dists;
                    for (IdType nid : neighbor->neighbors[l]) {
                        auto n_it = nodes_.find(nid);
                        if (n_it != nodes_.end()) {
                            double d = CosineDistance(neighbor->embedding, n_it->second->embedding);
                            dists.push_back({nid, d});
                        }
                    }

                    std::sort(dists.begin(), dists.end(),
                        [](const auto& a, const auto& b) { return a.second < b.second; });

                    neighbor->neighbors[l].clear();
                    for (size_t i = 0; i < std::min(M_, dists.size()); ++i) {
                        neighbor->neighbors[l].insert(dists[i].first);
                    }
                }
            }
        }

        if (!nearest.empty()) {
            current_point = nearest[0];
        }
    }

    if (level > max_level_global_) {
        max_level_global_ = level;
        entry_point_ = new_id;
    }

    nodes_[new_id] = std::move(new_node);
    doc_count_++;
}

void HnswIndex::Remove(IdType id) {
    std::unique_lock ul(mtx_);

    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        return;
    }

    Node* node_to_remove = it->second.get();

    for (int l = 0; l <= node_to_remove->level; ++l) {
        for (IdType neighbor_id : node_to_remove->neighbors[l]) {
            auto neighbor_it = nodes_.find(neighbor_id);
            if (neighbor_it != nodes_.end() &&
                static_cast<size_t>(l) < neighbor_it->second->neighbors.size()) {
                neighbor_it->second->neighbors[l].erase(id);
            }
        }
    }

    nodes_.erase(it);
    doc_count_--;
    storage_.Remove(id);

    if (entry_point_ == id && !nodes_.empty()) {
        entry_point_ = nodes_.begin()->first;

        for (const auto& [nid, node] : nodes_) {
            if (node->level > nodes_[entry_point_]->level) {
                entry_point_ = nid;
            }
        }
        max_level_global_ = nodes_[entry_point_]->level;
    } else if (nodes_.empty()) {
        max_level_global_ = -1;
        entry_point_ = 0;
    }
}

std::vector<SearchResult> HnswIndex::Search(const std::string& query, const SearchOptions& options) const {
    if (nodes_.empty() || options.top_k == 0) {
        return {};
    }

    std::vector<float> query_emb = embedder_.Embed(query);
    std::vector<IdType> nearest_ids = KnnSearch(query_emb, options.top_k, options.category);

    std::vector<SearchResult> results;
    results.reserve(nearest_ids.size());

    for (IdType id : nearest_ids) {
        std::optional<Ad> ad_opt = storage_.Get(id);
        if (!ad_opt.has_value()) {
            continue;
        }

        auto node_it = nodes_.find(id);
        if (node_it != nodes_.end()) {
            double dist = CosineDistance(query_emb, node_it->second->embedding);
            double score = 1.0 - dist;
            results.push_back({id, score});
        }
    }

    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) { return a.score > b.score; });

    if (options.offset >= results.size()) {
        return {};
    }

    auto from = results.begin() + options.offset;
    auto available = results.size() - options.offset;
    auto count = std::min(options.top_k, available);
    auto to = from + count;

    return {from, to};
}

std::optional<Ad> HnswIndex::Get(IdType ad_id) const {
    std::shared_lock sl(mtx_);
    return storage_.Get(ad_id);
}

IndexStats HnswIndex::Stats() const {
    std::shared_lock sl(mtx_);

    IndexStats stats;
    stats.documents_count = doc_count_;
    stats.categories_count = CalcCategories(storage_);
    stats.embedding_dimension = embedder_.Dimension();

    size_t memory = 0;
    for (const auto& [id, node] : nodes_) {
        memory += node->embedding.size() * sizeof(float);
        memory += node->neighbors.size() * sizeof(std::set<IdType>);
        for (const auto& neighbors_set : node->neighbors) {
            memory += neighbors_set.size() * sizeof(IdType);
        }
    }

    stats.memory_bytes = memory;
    return stats;
}

size_t HnswIndex::Size() const {
    std::shared_lock sl(mtx_);
    return doc_count_;
}

void HnswIndex::Clear() {
    std::unique_lock ul(mtx_);
    nodes_.clear();
    storage_.Clear();
    doc_count_ = 0;
    entry_point_ = 0;
    max_level_global_ = -1;
}
