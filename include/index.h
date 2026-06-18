#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ad.h"
#include "index_stats.h"
#include "search_options.h"
#include "search_result.h"
#include "storage.h"

enum class IndexType { InvertedBm25, FlatVector, Hnsw };

class Index {
   public:
    virtual ~Index() = default;

    virtual IndexType Type() const noexcept = 0;

    virtual void Add(const Ad& ad) = 0;

    virtual void Remove(IdType id) = 0;

    virtual std::vector<SearchResult> Search(
        const std::string& query, const SearchOptions& options) const = 0;

    virtual std::optional<Ad> Get(IdType ad_id) const = 0;

    virtual IndexStats Stats() const = 0;

    virtual size_t Size() const = 0;

    virtual bool Empty() const { return Size() == 0; }

    virtual void Clear() = 0;

   protected:
    explicit Index(DocumentStorage& storage)
        : storage_(storage) {}

    DocumentStorage& storage_;
};
