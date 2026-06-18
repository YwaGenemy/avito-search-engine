#include <benchmark/benchmark.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <vector>

#include "dataset_loader.h"
#include "flat_vector.h"
#include "storage.h"

namespace {

std::string ResolveDatasetPath() {
    const char* env_path = std::getenv("BENCHMARK_DATASET");
    if (env_path != nullptr && env_path[0] != '\0') {
        return std::string(env_path);
    }
    return "benchmarks/data/dataset.json";
}

const BenchmarkDataset& GetDataset() {
    static const BenchmarkDataset dataset = LoadDatasetFromJson(ResolveDatasetPath());
    return dataset;
}

}  // namespace

static void BM_FlatVectorSearch(benchmark::State& state) {
    const BenchmarkDataset* dataset = nullptr;
    try {
        dataset = &GetDataset();
    } catch (const std::exception& ex) {
        state.SkipWithError(ex.what());
        return;
    }

    if (dataset->ads.empty()) {
        state.SkipWithError("Dataset has no ads");
        return;
    }
    if (dataset->queries.empty()) {
        state.SkipWithError("Dataset has no queries");
        return;
    }

    DocumentStorage storage;
    FlatVectorIndex index(storage, 128);

    for (const Ad& ad : dataset->ads) {
        index.Add(ad);
    }

    std::size_t query_idx = 0;
    for (auto _ : state) {
        const BenchmarkQuery& query = dataset->queries[query_idx % dataset->queries.size()];
        ++query_idx;

        const std::vector<SearchResult> results = index.Search(query.text, query.options);
        benchmark::DoNotOptimize(results.size());
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_FlatVectorSearch)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
