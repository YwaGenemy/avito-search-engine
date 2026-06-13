#include "embedder.h"

#include <gtest/gtest.h>

TEST(EmbedderTest, ReturnsVectorWithExpectedDimension){
    Embedder embedder(32);

    const auto vector = embedder.Embed("iphone pro max");

    EXPECT_EQ(vector.size(), 32);
    EXPECT_EQ(embedder.Dimension(), 32);
}

TEST(EmbedderTest, SameTextReturnsSameVector){
    Embedder embedder(32);

    const auto first = embedder.Embed("iphone pro max");
    const auto second = embedder.Embed("iphone pro max");

    EXPECT_EQ(first, second);
}
