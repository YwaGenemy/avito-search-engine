#include "flat_vector.h"

#include <gtest/gtest.h>

TEST(FlatVectorTest, AddIncreasesSize){
    DocumentStorage storage;
    FlatVectorIndex index(storage);

    index.Add(Ad("iphone", "apple phone", "electronics"));

    EXPECT_EQ(index.Size(), 1);
}

TEST(FlatVectorTest, SearchFindsAddedAd){
    DocumentStorage storage;
    FlatVectorIndex index(storage);

    index.Add(Ad("iphone", "apple phone", "electronics"));

    SearchOptions options;
    const auto results = index.Search("iphone", options);

    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results.front().ad_id, 1);
}

TEST(FlatVectorTest, SearchUsesCategoryFilter){
    DocumentStorage storage;
    FlatVectorIndex index(storage);

    index.Add(Ad("iphone", "apple phone", "electronics"));
    index.Add(Ad("math", "linear algebra homework", "homework"));

    SearchOptions options;
    options.category = "homework";

    const auto results = index.Search("math", options);

    ASSERT_FALSE(results.empty());
    const auto ad = index.Get(results.front().ad_id);
    ASSERT_TRUE(ad.has_value());
    EXPECT_EQ(ad->category, "homework");
}

TEST(FlatVectorTest, ClearRemovesAllAds){
    DocumentStorage storage;
    FlatVectorIndex index(storage);

    index.Add(Ad("iphone", "apple phone", "electronics"));
    index.Clear();

    EXPECT_TRUE(index.Empty());
}

TEST(FlatVectorTest, SearchPaginatesAfterRanking){
    DocumentStorage storage;
    FlatVectorIndex index(storage);

    index.Add(Ad("iphone apple phone", "", "electronics"));
    index.Add(Ad("iphone case", "", "accessories"));
    index.Add(Ad("linear algebra", "", "education"));

    SearchOptions all_options;
    all_options.top_k = 10;
    const auto all = index.Search("iphone apple phone", all_options);
    ASSERT_EQ(all.size(), 3);
    EXPECT_EQ(all.front().ad_id, 1);

    SearchOptions page_options;
    page_options.top_k = 1;
    page_options.offset = 1;
    const auto page = index.Search("iphone apple phone", page_options);
    ASSERT_EQ(page.size(), 1);
    EXPECT_EQ(page.front().ad_id, all[1].ad_id);

    page_options.offset = 10;
    EXPECT_TRUE(index.Search("iphone apple phone", page_options).empty());
}

TEST(FlatVectorTest, RemoveUpdatesSearchGetAndSize){
    DocumentStorage storage;
    FlatVectorIndex index(storage);

    index.Add(Ad("delete marker", "", "temporary"));
    index.Add(Ad("keep marker", "", "permanent"));
    index.Remove(1);

    EXPECT_EQ(index.Size(), 1);
    EXPECT_FALSE(index.Get(1).has_value());
    const auto results = index.Search("marker", SearchOptions{});
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results.front().ad_id, 2);
}

TEST(FlatVectorTest, StatsDescribeIndexedDocuments){
    DocumentStorage storage;
    FlatVectorIndex index(storage, 32);

    index.Add(Ad("iphone", "phone", "electronics"));
    index.Add(Ad("charger", "cable", "accessories"));

    const auto stats = index.Stats();
    EXPECT_EQ(stats.documents_count, 2);
    EXPECT_EQ(stats.categories_count, 2);
    EXPECT_EQ(stats.embedding_dimension, 32);
    EXPECT_GT(stats.memory_bytes, 0);
}
