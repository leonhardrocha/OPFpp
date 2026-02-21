#include <gtest/gtest.h>
#include <cmath>
#include "ecs/Entity.h"
#include "ecs/EntityRegistry.h"
#include "ecs/Components.hpp"
#include "ecs/TrainSystem.hpp"
#include "ecs/SplitSystem.hpp"

class ECSTrainSystemTest : public ::testing::Test {
protected:
    ecs::EntityRegistry registry;
    ecs::TrainSystem train_system;

    // Helper: Create sample entities with features and labels
    std::vector<ecs::EntityId> createSampleEntities(
        const std::vector<std::vector<float>>& features,
        const std::vector<int>& labels) {
        
        std::vector<ecs::EntityId> sample_ids;
        for (size_t i = 0; i < features.size(); ++i) {
            auto sample = registry.create(ecs::EntityType::Sample);
            registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures(features[i]));
            registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(labels[i]));
            sample_ids.push_back(sample.id);
        }
        return sample_ids;
    }

    // Helper: Convert EntityId vector to int vector for CSamples
    std::vector<int> toIntVector(const std::vector<ecs::EntityId>& ids) {
        std::vector<int> result;
        for (auto id : ids) {
            result.push_back(static_cast<int>(id));
        }
        return result;
    }
};

// ============================================================================
// Goal 1: MST Prototype Selection Tests
// ============================================================================

TEST_F(ECSTrainSystemTest, MSTDetectsPrototypesAtClassBoundaries) {
    // Create 2 distinct classes with clear separation
    std::vector<std::vector<float>> class0_samples = {
        {0.0f, 0.0f},  // Class 0 cluster
        {0.1f, 0.1f},
        {0.2f, 0.0f}
    };
    std::vector<std::vector<float>> class1_samples = {
        {10.0f, 10.0f},  // Class 1 cluster (far from class 0)
        {10.1f, 10.0f},
        {10.0f, 10.1f}
    };

    std::vector<std::vector<float>> all_features = class0_samples;
    all_features.insert(all_features.end(), class1_samples.begin(), class1_samples.end());
    
    std::vector<int> all_labels = {0, 0, 0, 1, 1, 1};

    auto sample_ids = createSampleEntities(all_features, all_labels);

    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);
    
    // With 2 well-separated classes, expect at least 2 prototypes
    EXPECT_GE(model->prototypes.size(), 2);
}

TEST_F(ECSTrainSystemTest, MSTPrototypesHaveZeroPathValue) {
    std::vector<std::vector<float>> features = {{0.0f}, {1.0f}, {10.0f}};
    std::vector<int> labels = {0, 0, 1};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    // All prototypes should have pathval = 0.0
    for (int proto_idx : model->prototypes) {
        EXPECT_EQ(model->pathvalues[proto_idx], 0.0f);
    }
}

TEST_F(ECSTrainSystemTest, MSTPrototypesHaveNoPredecessors) {
    std::vector<std::vector<float>> features = {{0.0f}, {5.0f}, {10.0f}};
    std::vector<int> labels = {0, 1, 2};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    // All prototypes should have predecessor = -1
    for (int proto_idx : model->prototypes) {
        EXPECT_EQ(model->predecessors[proto_idx], -1);
    }
}

// ============================================================================
// Goal 2: IFT Label Propagation Tests
// ============================================================================

TEST_F(ECSTrainSystemTest, IFTPropagatesLabelsFromPrototypes) {
    // 3 samples, 2 classes: expect labels to propagate
    std::vector<std::vector<float>> features = {
        {0.0f}, {0.5f}, {10.0f}
    };
    std::vector<int> labels = {0, 0, 1};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    // All nodes should have valid labels (no -1)
    for (int label : model->node_labels) {
        EXPECT_GE(label, 0);
    }
}

TEST_F(ECSTrainSystemTest, IFTCostFunctionIsMaxOfPathvalAndWeight) {
    // Linear arrangement: [0] -- [1] -- [2]
    // Labels: 0, 0, 1
    std::vector<std::vector<float>> features = {
        {0.0f}, {5.0f}, {10.0f}
    };
    std::vector<int> labels = {0, 0, 1};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    // Non-prototypes should have pathval > 0
    bool found_non_prototype_with_nonzero_pathval = false;
    for (int i = 0; i < model->num_nodes; ++i) {
        bool is_prototype = std::find(model->prototypes.begin(), 
                                     model->prototypes.end(), i) != model->prototypes.end();
        if (!is_prototype && model->pathvalues[i] > 0.0f) {
            found_non_prototype_with_nonzero_pathval = true;
            break;
        }
    }
    EXPECT_TRUE(found_non_prototype_with_nonzero_pathval);
}

TEST_F(ECSTrainSystemTest, IFTPredecessorPointsTowardPrototype) {
    std::vector<std::vector<float>> features = {{0.0f}, {1.0f}, {10.0f}};
    std::vector<int> labels = {0, 0, 1};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    // For each non-prototype, predecessor should have lower or equal pathval
    for (int i = 0; i < model->num_nodes; ++i) {
        int pred = model->predecessors[i];
        if (pred != -1) {
            EXPECT_LE(model->pathvalues[pred], model->pathvalues[i]);
        }
    }
}

// ============================================================================
// Goal 3: Ordered Nodes Tests
// ============================================================================

TEST_F(ECSTrainSystemTest, OrderedNodesAreSortedByPathValue) {
    std::vector<std::vector<float>> features = {{0.0f}, {5.0f}, {10.0f}, {15.0f}};
    std::vector<int> labels = {0, 0, 1, 1};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    // Verify ascending order
    for (size_t i = 1; i < model->ordered_nodes.size(); ++i) {
        int prev_idx = model->ordered_nodes[i-1];
        int curr_idx = model->ordered_nodes[i];
        EXPECT_LE(model->pathvalues[prev_idx], model->pathvalues[curr_idx]);
    }
}

TEST_F(ECSTrainSystemTest, OrderedNodesContainsAllIndices) {
    std::vector<std::vector<float>> features = {{0.0f}, {1.0f}, {2.0f}};
    std::vector<int> labels = {0, 0, 1};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    EXPECT_EQ(model->ordered_nodes.size(), 3);
    
    std::vector<bool> seen(3, false);
    for (int idx : model->ordered_nodes) {
        ASSERT_GE(idx, 0);
        ASSERT_LT(idx, 3);
        seen[idx] = true;
    }
    
    for (bool s : seen) {
        EXPECT_TRUE(s);
    }
}

// ============================================================================
// Goal 4: Feature Storage Tests
// ============================================================================

TEST_F(ECSTrainSystemTest, ModelStoresAllTrainingFeatures) {
    std::vector<std::vector<float>> features = {
        {1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}
    };
    std::vector<int> labels = {0, 0, 1};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    // all_features should store all samples
    EXPECT_EQ(model->all_features.size(), 3);
    EXPECT_EQ(model->all_features[0], features[0]);
    EXPECT_EQ(model->all_features[1], features[1]);
    EXPECT_EQ(model->all_features[2], features[2]);
}

TEST_F(ECSTrainSystemTest, ModelStoresPrototypeFeatures) {
    std::vector<std::vector<float>> features = {
        {0.0f}, {0.1f}, {10.0f}
    };
    std::vector<int> labels = {0, 0, 1};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    // prototype_features should match prototypes
    EXPECT_EQ(model->prototype_features.size(), model->prototypes.size());
    
    for (size_t i = 0; i < model->prototypes.size(); ++i) {
        int proto_idx = model->prototypes[i];
        EXPECT_EQ(model->prototype_features[i], features[proto_idx]);
    }
}

// ============================================================================
// Goal 5: Single Sample Tests
// ============================================================================

TEST_F(ECSTrainSystemTest, SingleSampleCreatesTrivialModel) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures({5.5f, 6.6f}));
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(7));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(sample.id);
    ASSERT_NE(model, nullptr);
    
    EXPECT_EQ(model->prototypes.size(), 1);
    EXPECT_EQ(model->prototypes[0], 0);
    EXPECT_EQ(model->num_nodes, 1);
    EXPECT_EQ(model->node_labels[0], 7);
    EXPECT_EQ(model->pathvalues[0], 0.0f);
    EXPECT_EQ(model->predecessors[0], -1);
}

TEST_F(ECSTrainSystemTest, SingleSampleMetricsAlwaysAccurate) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(42));

    train_system.update(registry, 0.0);

    auto* metrics = registry.getComponent<ecs::CEvalMetrics>(sample.id);
    ASSERT_NE(metrics, nullptr);
    
    EXPECT_EQ(metrics->accuracy, 1.0f);
    EXPECT_EQ(metrics->num_samples_trained, 1);
    EXPECT_EQ(metrics->num_prototypes, 1);
    EXPECT_EQ(metrics->status, "trained");
}

// ============================================================================
// Goal 6: Multi-Class Tests
// ============================================================================

TEST_F(ECSTrainSystemTest, ThreeClassDatasetSelectsMultiplePrototypes) {
    std::vector<std::vector<float>> features = {
        {0.0f}, {0.1f},      // Class 0
        {5.0f}, {5.1f},      // Class 1
        {10.0f}, {10.1f}     // Class 2
    };
    std::vector<int> labels = {0, 0, 1, 1, 2, 2};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);

    // Expect prototypes from multiple classes
    EXPECT_GE(model->prototypes.size(), 2);
}

// ============================================================================
// Goal 7: Missing Components Tests
// ============================================================================

TEST_F(ECSTrainSystemTest, EntityMissingLabelIsSkipped) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures({1.0f}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(sample.id);
    EXPECT_EQ(model, nullptr);
}

TEST_F(ECSTrainSystemTest, EntityMissingFeaturesIsSkipped) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(1));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(sample.id);
    EXPECT_EQ(model, nullptr);
}

// ============================================================================
// Goal 8: Metrics Recording Tests
// ============================================================================

TEST_F(ECSTrainSystemTest, MetricsRecordsPrototypeCount) {
    std::vector<std::vector<float>> features = {{0.0f}, {1.0f}, {10.0f}};
    std::vector<int> labels = {0, 0, 1};
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    auto* metrics = registry.getComponent<ecs::CEvalMetrics>(subgraph.id);
    
    EXPECT_EQ(metrics->num_prototypes, static_cast<int>(model->prototypes.size()));
    EXPECT_EQ(metrics->num_samples_trained, model->num_nodes);
}

TEST_F(ECSTrainSystemTest, MetricsRecordsTrainedStatus) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(1));

    train_system.update(registry, 0.0);

    auto* metrics = registry.getComponent<ecs::CEvalMetrics>(sample.id);
    EXPECT_EQ(metrics->status, "trained");
}

// ============================================================================
// Goal 9: Large-Scale Data Tests
// ============================================================================

TEST_F(ECSTrainSystemTest, TrainSubgraphWith100Samples) {
    std::vector<std::vector<float>> features;
    std::vector<int> labels;
    
    // Create 100 samples in 3 clusters
    for (int i = 0; i < 100; ++i) {
        if (i < 30) {
            features.push_back({static_cast<float>(i) * 0.1f});
            labels.push_back(0);
        } else if (i < 60) {
            features.push_back({static_cast<float>(i) * 0.1f + 10.0f});
            labels.push_back(1);
        } else {
            features.push_back({static_cast<float>(i) * 0.1f + 20.0f});
            labels.push_back(2);
        }
    }
    
    auto sample_ids = createSampleEntities(features, labels);
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);
    
    EXPECT_EQ(model->num_nodes, 100);
    EXPECT_EQ(model->node_labels.size(), 100);
    EXPECT_EQ(model->pathvalues.size(), 100);
    EXPECT_EQ(model->predecessors.size(), 100);
    EXPECT_EQ(model->ordered_nodes.size(), 100);
}

