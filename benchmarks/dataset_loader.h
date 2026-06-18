#pragma once

#include <string>
#include <vector>

#include "ad.h"
#include "search_options.h"

struct BenchmarkQuery {
    std::string text;
    SearchOptions options;
    std::vector<IdType> relevant_ad_ids;
};

struct BenchmarkDataset {
    std::vector<Ad> ads;
    std::vector<BenchmarkQuery> queries;
};

BenchmarkDataset LoadDatasetFromJson(const std::string& path);