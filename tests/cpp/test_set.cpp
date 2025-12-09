#include <gtest/gtest.h>
#include "opf/set.hpp"

TEST(SetTest, InsertAndContains) {
    opf::Set<int> s;
    s.insert(10);
    s.insert(20);
    EXPECT_TRUE(s.contains(10));
    EXPECT_TRUE(s.contains(20));
    EXPECT_FALSE(s.contains(30));
}

TEST(SetTest, Erase) {
    opf::Set<int> s;
    s.insert(10);
    s.insert(20);
    s.erase(10);
    EXPECT_FALSE(s.contains(10));
    EXPECT_TRUE(s.contains(20));
}

TEST(SetTest, Size) {
    opf::Set<int> s;
    EXPECT_EQ(s.size(), 0);
    s.insert(10);
    EXPECT_EQ(s.size(), 1);
    s.insert(20);
    EXPECT_EQ(s.size(), 2);
    s.insert(20); // Insert duplicate
    EXPECT_EQ(s.size(), 2);
    s.erase(10);
    EXPECT_EQ(s.size(), 1);
}

TEST(SetTest, Empty) {
    opf::Set<int> s;
    EXPECT_TRUE(s.empty());
    s.insert(10);
    EXPECT_FALSE(s.empty());
    s.clear();
    EXPECT_TRUE(s.empty());
}

TEST(SetTest, Clear) {
    opf_Set<int> s;
    s.insert(10);
    s.insert(20);
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
    EXPECT_FALSE(s.contains(10));
}

TEST(SetTest, IntSet) {
    opf::IntSet s;
    s.insert(42);
    EXPECT_TRUE(s.contains(42));
    EXPECT_EQ(s.size(), 1);
}

TEST(SetTest, String) {
    opf::Set<std::string> s;
    s.insert("hello");
    s.insert("world");
    EXPECT_TRUE(s.contains("hello"));
    EXPECT_TRUE(s.contains("world"));
    EXPECT_FALSE(s.contains("test"));
    EXPECT_EQ(s.size(), 2);
}
