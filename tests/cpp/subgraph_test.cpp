#include "gtest/gtest.h"
#include "opf/Subgraph.hpp"
#include <map>
#include <numeric>

// Helper function to create a predictable test subgraph.
std::unique_ptr<opf::Subgraph> CreateTestSubgraph(size_t num_nodes, size_t num_labels) {
    if (num_nodes == 0 || num_labels == 0 || num_nodes % num_labels != 0) {
        throw std::invalid_argument("For this helper, num_nodes must be a non-zero multiple of num_labels.");
    }
    auto g = std::make_unique<opf::Subgraph>(num_nodes, 2, num_labels);
    size_t nodes_per_label = num_nodes / num_labels;
    for (size_t i = 0; i < num_nodes; ++i) {
        g->nodes[i].position = i;
        // Assign labels like 1, 1, ..., 2, 2, ...
        g->nodes[i].truelabel = (i / nodes_per_label) + 1;
    }
    return g;
}

// Helper to count the number of nodes for each label in a subgraph.
std::map<int, int> CountLabels(const opf::Subgraph& g) {
    std::map<int, int> counts;
    for (const auto& node : g.nodes) {
        counts[node.truelabel]++;
    }
    return counts;
}

// Test fixture to set up a common subgraph for multiple tests.
class SubgraphSplitTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a subgraph with 20 nodes, 2 labels (10 of label 1, 10 of label 2).
        subgraph = CreateTestSubgraph(20, 2);
    }

    std::unique_ptr<opf::Subgraph> subgraph;
};

TEST_F(SubgraphSplitTest, Handles50PercentSplit) {
    const size_t original_size = subgraph->nodes.size();

    // Perform the split. `subgraph` is modified to become the first part.
    auto testing_part = subgraph->Split(0.5f);

    // --- Assertions ---
    // 1. Total number of nodes should be conserved.
    EXPECT_EQ(subgraph->nodes.size() + testing_part->nodes.size(), original_size);

    // 2. Check sizes of the two resulting parts.
    // The first part should have max(1, 0.5 * 10) = 5 nodes for each of the 2 labels.
    EXPECT_EQ(subgraph->nodes.size(), 10);
    EXPECT_EQ(testing_part->nodes.size(), 10);

    // 3. Check for correct stratified sampling.
    auto training_labels = CountLabels(*subgraph);
    EXPECT_EQ(training_labels[1], 5);
    EXPECT_EQ(training_labels[2], 5);

    auto testing_labels = CountLabels(*testing_part);
    EXPECT_EQ(testing_labels[1], 5);
    EXPECT_EQ(testing_labels[2], 5);
}

TEST_F(SubgraphSplitTest, Handles70PercentSplit) {
    const size_t original_size = subgraph->nodes.size();

    auto testing_part = subgraph->Split(0.7f);

    EXPECT_EQ(subgraph->nodes.size() + testing_part->nodes.size(), original_size);

    // First part: max(1, 0.7 * 10) = 7 nodes for each of the 2 labels.
    EXPECT_EQ(subgraph->nodes.size(), 14);
    EXPECT_EQ(testing_part->nodes.size(), 6);

    auto training_labels = CountLabels(*subgraph);
    EXPECT_EQ(training_labels[1], 7);
    EXPECT_EQ(training_labels[2], 7);

    auto testing_labels = CountLabels(*testing_part);
    EXPECT_EQ(testing_labels[1], 3);
    EXPECT_EQ(testing_labels[2], 3);
}

TEST_F(SubgraphSplitTest, HandlesEdgeCaseSplitAtOne) {
    const size_t original_size = subgraph->nodes.size();
    auto testing_part = subgraph->Split(1.0f);

    // The first part should contain all nodes.
    EXPECT_EQ(subgraph->nodes.size(), original_size);
    // The second part should be empty.
    EXPECT_TRUE(testing_part->nodes.empty());
}

TEST(SubgraphSplitEmptyTest, DoesNotCrashOnEmptyGraph) {
    auto empty_graph = std::make_unique<opf::Subgraph>();

    // This should execute without throwing an exception or crashing.
    EXPECT_NO_THROW({
        auto second_part = empty_graph->Split(0.5f);
        EXPECT_TRUE(empty_graph->nodes.empty());
        EXPECT_TRUE(second_part->nodes.empty());
    });
}