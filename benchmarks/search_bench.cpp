#include <benchmark/benchmark.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <unordered_set>
#include <vector>

#include "bm25_index.h"
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

std::vector<IdType> ToRuntimeRelevantIds(const std::vector<IdType>& json_relevant_ids) {
    std::vector<IdType> runtime_relevant_ids;
    runtime_relevant_ids.reserve(json_relevant_ids.size());

    for (IdType json_id : json_relevant_ids) {
        runtime_relevant_ids.push_back(json_id + 1);
    }

    return runtime_relevant_ids;
}

SearchOptions RecallOptions(const SearchOptions& base, std::size_t k) {
    SearchOptions options = base;
    options.top_k = k;
    options.offset = 0;
    return options;
}

double ComputeRecallAtK(const std::vector<IdType>& relevant_ad_ids,
                        const std::vector<SearchResult>& results,
                        std::size_t k) {
    if (k == 0 || relevant_ad_ids.empty()) {
        return 0.0;
    }

    std::unordered_set<IdType> relevant_set;
    relevant_set.reserve(relevant_ad_ids.size());
    for (IdType id : relevant_ad_ids) {
        relevant_set.insert(id);
    }

    const std::size_t limit = std::min(k, results.size());
    std::size_t hits = 0;
    for (std::size_t i = 0; i < limit; ++i) {
        if (relevant_set.contains(results[i].ad_id)) {
            ++hits;
        }
    }

    return static_cast<double>(hits) /
           static_cast<double>(relevant_set.size());
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

    double recall10_sum = 0.0;
    double recall100_sum = 0.0;
    std::size_t query_idx = 0;
    for (auto _ : state) {
        const BenchmarkQuery& query = dataset->queries[query_idx % dataset->queries.size()];
        ++query_idx;

        const std::vector<IdType> relevant_runtime_ids =
            ToRuntimeRelevantIds(query.relevant_ad_ids);

        const std::vector<SearchResult> results10 =
            index.Search(query.text, RecallOptions(query.options, 10));
        const std::vector<SearchResult> results100 =
            index.Search(query.text, RecallOptions(query.options, 100));

        recall10_sum += ComputeRecallAtK(relevant_runtime_ids, results10, 10);
        recall100_sum += ComputeRecallAtK(relevant_runtime_ids, results100, 100);
        benchmark::DoNotOptimize(results10.size() + results100.size());
    }

    if (state.iterations() > 0) {
        const double avg_recall10 =
            recall10_sum / static_cast<double>(state.iterations());
        const double avg_recall100 =
            recall100_sum / static_cast<double>(state.iterations());

        state.counters["Recall@10"] =
            benchmark::Counter(avg_recall10, benchmark::Counter::kAvgThreads);
        state.counters["Recall@100"] =
            benchmark::Counter(avg_recall100, benchmark::Counter::kAvgThreads);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_Bm25Search(benchmark::State& state) {
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
    Bm25Index index(storage, 1.5, 0.75);

    for (const Ad& ad : dataset->ads) {
        index.Add(ad);
    }

    double recall10_sum = 0.0;
    double recall100_sum = 0.0;
    std::size_t query_idx = 0;
    for (auto _ : state) {
        const BenchmarkQuery& query = dataset->queries[query_idx % dataset->queries.size()];
        ++query_idx;

        const std::vector<IdType> relevant_runtime_ids =
            ToRuntimeRelevantIds(query.relevant_ad_ids);

        const std::vector<SearchResult> results10 =
            index.Search(query.text, RecallOptions(query.options, 10));
        const std::vector<SearchResult> results100 =
            index.Search(query.text, RecallOptions(query.options, 100));

        recall10_sum += ComputeRecallAtK(relevant_runtime_ids, results10, 10);
        recall100_sum += ComputeRecallAtK(relevant_runtime_ids, results100, 100);
        benchmark::DoNotOptimize(results10.size() + results100.size());
    }

    if (state.iterations() > 0) {
        const double avg_recall10 =
            recall10_sum / static_cast<double>(state.iterations());
        const double avg_recall100 =
            recall100_sum / static_cast<double>(state.iterations());

        state.counters["Recall@10"] =
            benchmark::Counter(avg_recall10, benchmark::Counter::kAvgThreads);
        state.counters["Recall@100"] =
            benchmark::Counter(avg_recall100, benchmark::Counter::kAvgThreads);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_HnswSearch(benchmark::State& state) {
    (void)state;
    state.SkipWithError("HNSW is not implemented yet");
}

BENCHMARK(BM_FlatVectorSearch)
    ->Unit(benchmark::kMillisecond)
    ->Threads(1)
    ->Threads(4)
    ->Threads(8);
BENCHMARK(BM_Bm25Search)
    ->Unit(benchmark::kMillisecond)
    ->Threads(1)
    ->Threads(4)
    ->Threads(8);
BENCHMARK(BM_HnswSearch)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Bm25Search)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_HnswSearch)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();