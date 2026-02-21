#include <gtest/gtest.h>
#include "ecs/Entity.h"
#include "ecs/EntityRegistry.h"
#include "ecs/Components.hpp"
#include "ecs/SplitSystem.hpp"

class ECSplitSystemTest : public ::testing::Test {
protected:
    ecs::EntityRegistry registry;
    ecs::SplitSystem split_system;
};

TEST_F(ECSplitSystemTest, CreateDatasetForSplit) {
    auto dataset = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset.id, ecs::CIOPath("/path/to/data.dat"));
    
    EXPECT_TRUE(registry.hasComponent<ecs::CIOPath>(dataset.id));
    auto *path = registry.getComponent<ecs::CIOPath>(dataset.id);
    EXPECT_EQ(path->path, "/path/to/data.dat");
}

TEST_F(ECSplitSystemTest, SplitDatasetCreateSubgraphs) {
    // Create dataset
    auto dataset = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset.id, ecs::CIOPath("/data/boat.dat"));
    
    // Add split parameters
    registry.addComponent<ecs::CSplitParams>(
        dataset.id, 
        ecs::CSplitParams(0.5f, 0.0f, 0.5f)  // 50% train, 50% test
    );
    
    // Run split system
    split_system.update(registry, 0.0);
    
    // Query for subgraph entities
    auto subgraphs = registry.view<ecs::CSamples, ecs::CFlags>();
    EXPECT_EQ(subgraphs.size(), 2);  // train + test (no eval)
}

TEST_F(ECSplitSystemTest, SplitSystemCreatesTrainSubgraph) {
    auto dataset = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset.id, ecs::CIOPath("/data/boat.dat"));
    registry.addComponent<ecs::CSplitParams>(dataset.id, ecs::CSplitParams(0.6f, 0.0f, 0.4f));
    
    split_system.update(registry, 0.0);
    
    auto subgraphs = registry.view<ecs::CSamples, ecs::CFlags>();
    
    // Find training subgraph
    ecs::EntityId train_id = 0;
    for (auto id : subgraphs) {
        auto *flags = registry.getComponent<ecs::CFlags>(id);
        if (flags && flags->isTraining) {
            train_id = id;
            break;
        }
    }
    
    EXPECT_NE(train_id, 0);
    
    auto *samples = registry.getComponent<ecs::CSamples>(train_id);
    EXPECT_NE(samples, nullptr);
    // 60% of 100 samples = 60
    EXPECT_EQ(samples->indices.size(), 60);
}

TEST_F(ECSplitSystemTest, SplitSystemCreatesTestSubgraph) {
    auto dataset = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset.id, ecs::CIOPath("/data/boat.dat"));
    registry.addComponent<ecs::CSplitParams>(dataset.id, ecs::CSplitParams(0.6f, 0.0f, 0.4f));
    
    split_system.update(registry, 0.0);
    
    auto subgraphs = registry.view<ecs::CSamples, ecs::CFlags>();
    
    // Find test subgraph
    ecs::EntityId test_id = 0;
    for (auto id : subgraphs) {
        auto *flags = registry.getComponent<ecs::CFlags>(id);
        if (flags && flags->isTesting) {
            test_id = id;
            break;
        }
    }
    
    EXPECT_NE(test_id, 0);
    
    auto *samples = registry.getComponent<ecs::CSamples>(test_id);
    EXPECT_NE(samples, nullptr);
    // 40% of 100 samples = 40
    EXPECT_EQ(samples->indices.size(), 40);
}

TEST_F(ECSplitSystemTest, SplitSystemWithEvalSet) {
    auto dataset = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset.id, ecs::CIOPath("/data/cone-torus.dat"));
    registry.addComponent<ecs::CSplitParams>(dataset.id, ecs::CSplitParams(0.5f, 0.2f, 0.3f));
    
    split_system.update(registry, 0.0);
    
    auto subgraphs = registry.view<ecs::CSamples, ecs::CFlags>();
    
    // Should have 3 subgraphs: train, eval, test
    int train_count = 0, eval_count = 0, test_count = 0;
    for (auto id : subgraphs) {
        auto *flags = registry.getComponent<ecs::CFlags>(id);
        if (flags) {
            if (flags->isTraining) train_count++;
            if (flags->isEvaluation) eval_count++;
            if (flags->isTesting) test_count++;
        }
    }
    
    EXPECT_EQ(train_count, 1);
    EXPECT_EQ(eval_count, 1);
    EXPECT_EQ(test_count, 1);
}

TEST_F(ECSplitSystemTest, SampleIndicesNoDuplicate) {
    auto dataset = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset.id, ecs::CIOPath("/data/saturn.dat"));
    registry.addComponent<ecs::CSplitParams>(dataset.id, ecs::CSplitParams(0.4f, 0.3f, 0.3f));
    
    split_system.update(registry, 0.0);
    
    auto subgraphs = registry.view<ecs::CSamples>();
    
    std::vector<int> all_indices;
    for (auto id : subgraphs) {
        auto *samples = registry.getComponent<ecs::CSamples>(id);
        if (samples) {
            all_indices.insert(all_indices.end(), 
                               samples->indices.begin(), 
                               samples->indices.end());
        }
    }
    
    // Sort and check for uniqueness
    std::sort(all_indices.begin(), all_indices.end());
    auto last = std::unique(all_indices.begin(), all_indices.end());
    EXPECT_EQ(all_indices.end() - last, 0);  // No duplicates
    
    // Should cover all 100 samples
    EXPECT_EQ(all_indices.size(), 100);
}

TEST_F(ECSplitSystemTest, MultipleDatasetsSplitIndependently) {
    // Create two datasets
    auto dataset1 = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset1.id, ecs::CIOPath("/data/data1.dat"));
    registry.addComponent<ecs::CSplitParams>(dataset1.id, ecs::CSplitParams(0.7f, 0.0f, 0.3f));
    
    auto dataset2 = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset2.id, ecs::CIOPath("/data/data2.dat"));
    registry.addComponent<ecs::CSplitParams>(dataset2.id, ecs::CSplitParams(0.5f, 0.2f, 0.3f));
    
    split_system.update(registry, 0.0);
    
    // Should have at least 4 subgraphs (2 for dataset1, 3 for dataset2)
    auto subgraphs = registry.view<ecs::CSamples>();
    EXPECT_GE(subgraphs.size(), 4);
}
