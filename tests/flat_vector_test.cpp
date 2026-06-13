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
