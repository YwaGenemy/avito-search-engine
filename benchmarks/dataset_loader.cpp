#include "dataset_loader.h"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace {

const nlohmann::json& RequireField(const nlohmann::json& object, const char* key) {
    if (!object.contains(key)) {
        throw std::runtime_error(std::string("Missing JSON field: ") + key);
    }
    return object.at(key);
}

}  // namespace

BenchmarkDataset LoadDatasetFromJson(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open dataset file: " + path);
    }

    nlohmann::json root;
    input >> root;

    BenchmarkDataset dataset;

    const nlohmann::json& ads_json = RequireField(root, "ads");
    if (!ads_json.is_array()) {
        throw std::runtime_error("'ads' must be an array");
    }

    dataset.ads.reserve(ads_json.size());
    for (const nlohmann::json& ad_json : ads_json) {
        const std::string title = RequireField(ad_json, "title").get<std::string>();
        const std::string description =
            RequireField(ad_json, "description").get<std::string>();
        const std::string category = RequireField(ad_json, "category").get<std::string>();

        dataset.ads.emplace_back(title, description, category);
    }

    const nlohmann::json& queries_json = RequireField(root, "queries");
    if (!queries_json.is_array()) {
        throw std::runtime_error("'queries' must be an array");
    }

    dataset.queries.reserve(queries_json.size());
    for (const nlohmann::json& query_json : queries_json) {
        BenchmarkQuery query;
        query.text = RequireField(query_json, "text").get<std::string>();

        if (query_json.contains("category") && !query_json["category"].is_null()) {
            query.options.category = query_json["category"].get<std::string>();
        }

        if (query_json.contains("top_k")) {
            query.options.top_k = query_json["top_k"].get<std::size_t>();
        }
        if (query_json.contains("offset")) {
            query.options.offset = query_json["offset"].get<std::size_t>();
        }

        if (query_json.contains("relevant_ad_ids")) {
            query.relevant_ad_ids =
                query_json["relevant_ad_ids"].get<std::vector<IdType>>();
        }

        dataset.queries.push_back(std::move(query));
    }

    return dataset;
}