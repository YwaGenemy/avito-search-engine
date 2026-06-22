#include "flat_vector.h"

#include <algorithm>
#include <mutex>
#include <unordered_set>

namespace {
std::size_t CalcCategories(const DocumentStorage& storage){
    std::unordered_set<std::string> us_cat;
    for(auto& [id, ad]: storage.All()){ us_cat.insert(ad.category); }
    return us_cat.size();
}

} //namespace

FlatVectorIndex::FlatVectorIndex(DocumentStorage &storage, size_t emb_dimension):
    Index(storage),
    embedder_(emb_dimension) {}

IndexType FlatVectorIndex::Type() const noexcept {
    return IndexType::FlatVector;
}

void FlatVectorIndex::Add(const Ad &ad) {
    std::vector<float> embedding = embedder_.Embed(ad.Text());

    std::unique_lock ul(mtx_);
    embeddings_[storage_.Add(ad)] = std::move(embedding);
}

void FlatVectorIndex::Remove(IdType id){
    std::unique_lock ul(mtx_);

    storage_.Remove(id);
    embeddings_.erase(id);
}

std::vector<SearchResult> FlatVectorIndex::Search(const std::string &query, const SearchOptions &options) const {
    std::vector<SearchResult> result;
    std::vector<float> embedding = embedder_.Embed(query);


    std::shared_lock sl(mtx_);

    for(auto& [id, temp_emb] : embeddings_){
        std::optional<Ad> ad = storage_.Get(id);
        if(!ad.has_value())continue;
        if(options.category.has_value() && ad->category != *options.category)continue;


        double score = 0.0;
        for(size_t i = 0;i < embedder_.Dimension();i++){
            score += temp_emb[i] * embedding[i]; // скаляр
        }
        if(score == 0.0)continue;
        result.push_back({id, score});
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b){return a.score>b.score;});
    if(options.offset >= result.size()){ return {}; }

    auto from = result.begin() + options.offset;
    auto count = std::min(options.top_k, result.size() - options.offset);

    
    return {from, from+count};
}

std::optional<Ad> FlatVectorIndex::Get(IdType ad_id) const {
    std::shared_lock sl(mtx_);
    return storage_.Get(ad_id);
}

IndexStats FlatVectorIndex::Stats() const {
    IndexStats is;

    {
        std::shared_lock sl(mtx_);
        is.categories_count = CalcCategories(storage_);
        is.documents_count = storage_.Size();
        is.embedding_dimension = embedder_.Dimension();
        is.memory_bytes = embeddings_.size() * embedder_.Dimension() * sizeof(float);
    }

    return is;
}

size_t FlatVectorIndex::Size() const {
    std::shared_lock sl(mtx_);
    return storage_.Size();
}

void FlatVectorIndex::Clear(){
    std::unique_lock ul(mtx_);
    storage_.Clear();
    embeddings_.clear();
}
