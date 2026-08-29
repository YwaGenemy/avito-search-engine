#include "storage.h"

#include <gtest/gtest.h>

TEST(DocumentStorageTest, AddGetAndRemoveDocument) {
    DocumentStorage storage;

    const IdType id = storage.Add(Ad{"iphone", "phone", "electronics"});

    EXPECT_EQ(id, 1);
    ASSERT_TRUE(storage.Get(id).has_value());
    EXPECT_EQ(storage.Get(id)->GetID(), id);
    EXPECT_EQ(storage.Size(), 1);
    EXPECT_TRUE(storage.Remove(id));
    EXPECT_FALSE(storage.Get(id).has_value());
    EXPECT_TRUE(storage.Empty());
}

TEST(DocumentStorageTest, ClearRemovesDocumentsAndResetsIds) {
    DocumentStorage storage;
    storage.Add(Ad{"first", "", "category"});
    storage.Add(Ad{"second", "", "category"});

    storage.Clear();

    EXPECT_TRUE(storage.Empty());
    EXPECT_EQ(storage.Add(Ad{"after clear", "", "category"}), 1);
}
