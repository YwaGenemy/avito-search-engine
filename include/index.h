#pragma once

#include "ad.h"
#include "search_options.h"
#include "search_result.h"

#include <string>
#include <vector>

class Index {
public:
    virtual ~Index() = default;

    virtual void Add(const Ad& ad) = 0;

    virtual std::vector<SearchResult> Search(
        const std::string& query,
        const SearchOptions& options
    ) const = 0;
};
