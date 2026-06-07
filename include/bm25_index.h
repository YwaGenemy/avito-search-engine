#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "index.h"

class Bm25Index final : public Index {
   public:
    Bm25Index() = default;
    Bm25Index(double k1, double b);

    IndexType Type() const noexcept override;

    void Add(Ad& ad) override;

    void Remove(IdType ad_id);

    std::vector<SearchResult> Search(
        const std::string& query, const SearchOptions& options) const override;

    std::optional<Ad> Get(uint64_t ad_id) const override;

    IndexStats Stats() const override;

    std::size_t Size() const override;

    void Clear() override;

   private:
    using TermFrequencyByDocument = std::unordered_map<IdType, size_t>;

    bool MatchesCategory(IdType ad_id, const SearchOptions& options) const;
    double ScoreTerm(size_t term_frequency, size_t document_frequency,
                     size_t document_length) const;

    double k1_ = 1.5;
    double b_ = 0.75;

    std::unordered_map<std::string, TermFrequencyByDocument> postings_;
    std::unordered_map<std::string, size_t> category_counts_;
    std::unordered_map<IdType, size_t> document_lengths;
    size_t total_document_length_ = 0;
    size_t last_id = 0;
};
