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

    void Add(const Ad& ad) override;

    std::vector<SearchResult> Search(
        const std::string& query, const SearchOptions& options) const override;

    std::optional<Ad> Get(int ad_id) const override;

    IndexStats Stats() const override;

    std::size_t Size() const override;

    void Clear() override;

   private:
    using TermFrequencyByDocument = std::unordered_map<int, size_t>;

    static std::vector<std::string> Tokenize(const std::string& text);

    void RemoveFromIndex(int ad_id);
    bool MatchesCategory(int ad_id, const SearchOptions& options) const;
    double ScoreTerm(size_t term_frequency, size_t document_frequency,
                     size_t document_length) const;

    double k1_ = 1.5;
    double b_ = 0.75;

    std::unordered_map<int, Ad> ads_;
    std::unordered_map<std::string, TermFrequencyByDocument> postings_;
    std::unordered_map<std::string, size_t> category_counts_;
    size_t total_document_length_ = 0;
};
