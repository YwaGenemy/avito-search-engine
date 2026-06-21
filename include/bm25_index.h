#pragma once

#include <cstddef>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "index.h"

class Bm25Index final : public Index {
   public:
    Bm25Index(DocumentStorage& storage, double k1, double b);

    IndexType Type() const noexcept override;

    void Add(const Ad& ad) override;

    void Remove(IdType ad_id);

    std::vector<SearchResult> Search(
        const std::string& query, const SearchOptions& options) const override;

    std::optional<Ad> Get(IdType ad_id) const override;

    IndexStats Stats() const override;

    size_t Size() const override { return document_lengths.size(); }

    void Clear() override;

   private:
    using TermFrequencyByDocument = std::unordered_map<IdType, size_t>;

    bool MatchesCategory(IdType ad_id, const SearchOptions& options) const;
    double ScoreTerm(size_t term_frequency, size_t document_frequency,
                     size_t document_length) const;

    double k1_ = 1.5;
    double b_ = 0.75;

    mutable std::shared_mutex mutex_;

    std::unordered_map<std::string, TermFrequencyByDocument> postings_;
    std::unordered_map<IdType, size_t> document_lengths;
    std::unordered_set<std::string> category_counts_;
    size_t total_document_length_ = 0;
};
