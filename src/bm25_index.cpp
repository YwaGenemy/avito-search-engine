#include "bm25_index.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numeric>
#include <stdexcept>

Bm25Index::Bm25Index(double k1, double b) : k1_(k1), b_(b) {
    if (k1_ <= 0.0) {
        throw std::invalid_argument("BM25 k1 must be positive");
    }
    if (b_ < 0.0 || b_ > 1.0) {
        throw std::invalid_argument("BM25 b must be in [0, 1]");
    }
}

IndexType Bm25Index::Type() const noexcept { return IndexType::InvertedBm25; }

void Bm25Index::Add(const Ad& ad) {
    if (ads_.contains(ad.id)) {
        RemoveFromIndex(ad.id);
    }

    const auto terms = Tokenize(ad.Text());
    std::unordered_map<std::string, size_t> term_frequencies;

    for (const auto& term : terms) {
        ++term_frequencies[term];
    }

    for (const auto& [term, frequency] : term_frequencies) {
        postings_[term][ad.id] = frequency;
    }

    ads_[ad.id] = ad;
    total_document_length_ += terms.size();
    ++category_counts_[ad.category];
}

std::vector<SearchResult> Bm25Index::Search(
    const std::string& query, const SearchOptions& options) const {
    if (ads_.empty() || options.top_k == 0) {
        return {};
    }

    const auto query_terms = Tokenize(query);
    if (query_terms.empty()) {
        return {};
    }

    std::unordered_map<int, double> scores;

    for (const auto& term : query_terms) {
        const auto postings_it = postings_.find(term);
        if (postings_it == postings_.end()) {
            continue;
        }

        const auto& term_postings = postings_it->second;
        const auto document_frequency = term_postings.size();

        for (const auto& [ad_id, term_frequency] : term_postings) {
            if (!MatchesCategory(ad_id, options)) {
                continue;
            }

            const auto length_it = ads_.find(ad_id);
            if (length_it == ads_.end()) {
                continue;
            }

            scores[ad_id] += ScoreTerm(term_frequency, document_frequency,
                                       length_it->second.GetTermsCount());
        }
    }

    std::vector<SearchResult> results;
    results.reserve(scores.size());

    for (const auto& [ad_id, score] : scores) {
        results.push_back({ad_id, score});
    }

    std::sort(results.begin(), results.end(),
              [](const auto& lhs, const auto& rhs) {
                  if (lhs.score == rhs.score) {
                      return lhs.ad_id < rhs.ad_id;
                  }
                  return lhs.score > rhs.score;
              });

    if (options.offset >= results.size()) {
        return {};
    }

    const auto from = results.begin() + options.offset;
    const auto available = results.size() - options.offset;
    const auto count = std::min(options.top_k, available);
    const auto to = from + count;

    return {from, to};
}

std::optional<Ad> Bm25Index::Get(int ad_id) const {
    const auto it = ads_.find(ad_id);
    if (it == ads_.end()) {
        return std::nullopt;
    }

    return it->second;
}

IndexStats Bm25Index::Stats() const {
    std::size_t postings_count = 0;
    for (const auto& [word, documents] : postings_) {
        postings_count += documents.size();
    }

    IndexStats stats;
    stats.documents_count = ads_.size();
    stats.categories_count = category_counts_.size();
    stats.embedding_dimension = 0;
    stats.memory_bytes = ads_.size() * sizeof(Ad) +
                         postings_.size() * sizeof(std::string) +
                         postings_count * (sizeof(int) + sizeof(size_t));

    return stats;
}

std::size_t Bm25Index::Size() const { return ads_.size(); }

void Bm25Index::Clear() {
    ads_.clear();
    postings_.clear();
    category_counts_.clear();
    total_document_length_ = 0;
}

std::vector<std::string> Bm25Index::Tokenize(const std::string& text) {
    std::vector<std::string> terms;
    std::string current;

    for (const unsigned char ch : text) {
        if (std::isalnum(ch)) {
            current.push_back(static_cast<char>(std::tolower(ch)));
        } else if (!current.empty()) {
            terms.push_back(current);
            current.clear();
        }
    }

    if (!current.empty()) {
        terms.push_back(current);
    }

    return terms;
}

void Bm25Index::RemoveFromIndex(int ad_id) {
    const auto ad_it = ads_.find(ad_id);
    if (ad_it == ads_.end()) {
        return;
    }

    const auto length = ad_it->second.GetTermsCount();
    total_document_length_ -= length;

    for (auto it = postings_.begin(); it != postings_.end();) {
        it->second.erase(ad_id);
        if (it->second.empty()) {
            it = postings_.erase(it);
        } else {
            ++it;
        }
    }

    const auto category_it = category_counts_.find(ad_it->second.category);
    if (category_it != category_counts_.end()) {
        --category_it->second;
        if (category_it->second == 0) {
            category_counts_.erase(category_it);
        }
    }

    ads_.erase(ad_it);
}

bool Bm25Index::MatchesCategory(int ad_id, const SearchOptions& options) const {
    if (!options.category.has_value()) {
        return true;
    }

    const auto ad_it = ads_.find(ad_id);
    return ad_it != ads_.end() && ad_it->second.category == *options.category;
}

double Bm25Index::ScoreTerm(size_t term_frequency, size_t document_frequency,
                            size_t document_length) const {
    const double documents_count = ads_.size();
    const double df = document_frequency;
    const double tf = term_frequency;
    const double dl = document_length;
    const double average_document_length =
        total_document_length_ / documents_count;

    if (average_document_length == 0.0) {
        return 0.0;
    }

    const double IDF =
        std::log(1.0 + (documents_count - df + 0.5) / (df + 0.5));
    const double length_penalty = 1.0 - b_ + b_ * dl / average_document_length;

    return IDF * (tf * (k1_ + 1.0)) / (tf + k1_ * length_penalty);
}
