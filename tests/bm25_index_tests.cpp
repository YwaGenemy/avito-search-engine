#include "bm25_index.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace {

Bm25Index MakeIndex(DocumentStorage& storage) {
    return Bm25Index(storage, 1.5, 0.75);
}

SearchOptions Options(std::size_t top_k = 10, std::size_t offset = 0,
                      std::optional<std::string> category = std::nullopt) {
    SearchOptions options;
    options.top_k = top_k;
    options.offset = offset;
    options.category = std::move(category);
    return options;
}

IdType FindSingleId(Index& index, const std::string& query) {
    const auto results = index.Search(query, Options());
    EXPECT_EQ(results.size(), 1) << "query: " << query;

    if (results.empty()) {
        return 0;
    }

    return results.front().ad_id;
}

}  // namespace

TEST(Bm25IndexTest, ConstructorAndInitialState) {
    DocumentStorage storage;
    Bm25Index index = MakeIndex(storage);
    Index& as_index = index;

    EXPECT_EQ(as_index.Type(), IndexType::InvertedBm25);
    EXPECT_TRUE(as_index.Empty());
    EXPECT_EQ(as_index.Size(), 0);
    EXPECT_TRUE(as_index.Search("iphone", Options()).empty());
    EXPECT_FALSE(as_index.Get(1).has_value());

    const auto stats = as_index.Stats();
    EXPECT_EQ(stats.documents_count, 0);
    EXPECT_EQ(stats.categories_count, 0);
    EXPECT_EQ(stats.embedding_dimension, 0);

    EXPECT_THROW(Bm25Index(storage, 0.0, 0.75), std::invalid_argument);
    EXPECT_THROW(Bm25Index(storage, -1.0, 0.75), std::invalid_argument);
    EXPECT_THROW(Bm25Index(storage, 1.5, -0.1), std::invalid_argument);
    EXPECT_THROW(Bm25Index(storage, 1.5, 1.1), std::invalid_argument);
}

TEST(Bm25IndexTest, AddSearchGetAndStats) {
    DocumentStorage storage;
    Bm25Index index = MakeIndex(storage);
    Index& as_index = index;

    as_index.Add(
        Ad{"iPhone 13", "alphaunique phone in good condition", "electronics"});

    EXPECT_FALSE(as_index.Empty());
    EXPECT_EQ(as_index.Size(), 1);

    const auto id = FindSingleId(as_index, "alphaunique");
    ASSERT_NE(id, 0);

    const auto ad = as_index.Get(id);
    ASSERT_TRUE(ad.has_value());
    EXPECT_EQ(ad->GetID(), id);
    EXPECT_EQ(ad->title, "iPhone 13");
    EXPECT_EQ(ad->description, "alphaunique phone in good condition");
    EXPECT_EQ(ad->category, "electronics");
    EXPECT_FALSE(as_index.Get(id + 1000).has_value());

    const auto stats = as_index.Stats();
    EXPECT_EQ(stats.documents_count, 1);
    EXPECT_EQ(stats.categories_count, 1);
    EXPECT_EQ(stats.embedding_dimension, 0);
    EXPECT_GT(stats.memory_bytes, 0);
}

TEST(Bm25IndexTest, SearchIsCaseInsensitiveAndIgnoresPunctuation) {
    DocumentStorage storage;
    Bm25Index index = MakeIndex(storage);
    Index& as_index = index;

    as_index.Add(Ad{"Apple iPhone-13", "Nearly new", "electronics"});

    const auto upper_case = as_index.Search("APPLE", Options());
    ASSERT_EQ(upper_case.size(), 1);

    const auto punctuation = as_index.Search("iphone!!! 13", Options());
    ASSERT_EQ(punctuation.size(), 1);
    EXPECT_EQ(upper_case.front().ad_id, punctuation.front().ad_id);
}

TEST(Bm25IndexTest, SearchSortsByBm25ScoreDescending) {
    DocumentStorage storage;
    Bm25Index index = MakeIndex(storage);
    Index& as_index = index;

    as_index.Add(Ad{"apple apple apple", "", "fruit"});
    as_index.Add(Ad{"apple", "", "fruit"});

    const auto results = as_index.Search("apple", Options());
    ASSERT_EQ(results.size(), 2);
    EXPECT_GE(results[0].score, results[1].score);

    const auto first = as_index.Get(results[0].ad_id);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->title, "apple apple apple");
}

TEST(Bm25IndexTest, SearchReturnsEmptyForMissingEmptyAndZeroLimitQueries) {
    DocumentStorage storage;
    Bm25Index index = MakeIndex(storage);
    Index& as_index = index;

    as_index.Add(Ad{"iPhone 13", "phone", "electronics"});

    EXPECT_TRUE(as_index.Search("missing", Options()).empty());
    EXPECT_TRUE(as_index.Search("!!!", Options()).empty());
    EXPECT_TRUE(as_index.Search("iphone", Options(0)).empty());
}

TEST(Bm25IndexTest, CategoryFilteringWorks) {
    DocumentStorage storage;
    Bm25Index index = MakeIndex(storage);
    Index& as_index = index;

    as_index.Add(Ad{"iPhone", "phone", "electronics"});
    as_index.Add(Ad{"iPhone case", "phone accessory", "accessories"});
    as_index.Add(Ad{"iPhone repair", "phone service", "services"});

    const auto accessories =
        as_index.Search("iphone", Options(10, 0, std::string{"accessories"}));

    ASSERT_EQ(accessories.size(), 1);

    const auto ad = as_index.Get(accessories.front().ad_id);
    ASSERT_TRUE(ad.has_value());
    EXPECT_EQ(ad->category, "accessories");

    const auto missing_category =
        as_index.Search("iphone", Options(10, 0, std::string{"real-estate"}));
    EXPECT_TRUE(missing_category.empty());
}

TEST(Bm25IndexTest, PaginationUsesOffsetAndTopKAfterSorting) {
    DocumentStorage storage;
    Bm25Index index = MakeIndex(storage);
    Index& as_index = index;

    as_index.Add(Ad{"camera camera camera", "", "electronics"});
    as_index.Add(Ad{"camera camera", "", "electronics"});
    as_index.Add(Ad{"camera", "", "electronics"});

    const auto all = as_index.Search("camera", Options(10, 0));
    ASSERT_EQ(all.size(), 3);

    const auto first_page = as_index.Search("camera", Options(2, 0));
    ASSERT_EQ(first_page.size(), 2);
    EXPECT_EQ(first_page[0].ad_id, all[0].ad_id);
    EXPECT_EQ(first_page[1].ad_id, all[1].ad_id);

    const auto second_item = as_index.Search("camera", Options(1, 1));
    ASSERT_EQ(second_item.size(), 1);
    EXPECT_EQ(second_item.front().ad_id, all[1].ad_id);

    const auto beyond_end = as_index.Search("camera", Options(10, 10));
    EXPECT_TRUE(beyond_end.empty());
}

TEST(Bm25IndexTest, RemoveUpdatesSearchGetSizeAndStats) {
    DocumentStorage storage;
    Bm25Index index = MakeIndex(storage);
    Index& as_index = index;

    as_index.Add(Ad{"deletemarker", "temporary phone", "electronics"});
    as_index.Add(Ad{"keepmarker", "permanent phone", "electronics"});

    const auto delete_id = FindSingleId(as_index, "deletemarker");
    ASSERT_NE(delete_id, 0);

    EXPECT_EQ(as_index.Size(), 2);
    as_index.Remove(delete_id);

    EXPECT_EQ(as_index.Size(), 1);
    EXPECT_FALSE(as_index.Get(delete_id).has_value());
    EXPECT_TRUE(as_index.Search("deletemarker", Options()).empty());
    EXPECT_EQ(as_index.Search("keepmarker", Options()).size(), 1);

    const auto stats = as_index.Stats();
    EXPECT_EQ(stats.documents_count, 1);
    EXPECT_EQ(stats.categories_count, 1);

    as_index.Remove(delete_id);
    as_index.Remove(123456789);
    EXPECT_EQ(as_index.Size(), 1);
}
