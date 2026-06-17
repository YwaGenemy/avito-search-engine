#include <benchmark/benchmark.h>

#include <array>
#include <string>
#include <vector>

#include "ad.h"
#include "flat_vector.h"
#include "search_options.h"
#include "storage.h"

namespace {

std::vector<Ad> BuildSyntheticAds(std::size_t count) {
    const std::array<std::string, 5> categories = {
        "phones", "cars", "furniture", "clothes", "realty"};

    const std::array<std::string, 5> items = {
        "iPhone 13", "BMW X5", "corner sofa", "winter jacket", "studio"};

    std::vector<Ad> ads;
    ads.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        const std::string& category = categories[i % categories.size()];
        const std::string& item = items[i % items.size()];

        ads.emplace_back(
            "Sell " + item,
            item + " synthetic ad #" + std::to_string(i),
            category
        );
    }

    return ads;
}

std::vector<std::string> BuildSyntheticQueries() {
    return {
        "iphone",
        "bmw",
        "sofa",
        "jacket",
        "studio",
        "cheap smartphone",
        "family car"
    };
}

}  // namespace

static void BM_FlatVectorSearch(benchmark::State& state) {
    const std::size_t corpus_size = static_cast<std::size_t>(state.range(0));

    DocumentStorage storage;
    FlatVectorIndex index(storage, 128);

    const std::vector<Ad> ads = BuildSyntheticAds(corpus_size);
    for (const Ad& ad : ads) {
        index.Add(ad);
    }

    const std::vector<std::string> queries = BuildSyntheticQueries();
    SearchOptions options;
    options.top_k = 10;

    std::size_t query_idx = 0;
    for (auto _ : state) {
        const std::string& query = queries[query_idx % queries.size()];
        ++query_idx;

        const std::vector<SearchResult> results = index.Search(query, options);
        benchmark::DoNotOptimize(results.size());
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_FlatVectorSearch)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
