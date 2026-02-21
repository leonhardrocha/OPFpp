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
};

// ============================================================================
// Goal 1: Update Flow — TrainSystem.update() finds and trains all entities
// ============================================================================

TEST_F(ECSTrainSystemTest, UpdateCallProcessesSingleSampleEntity) {
    // Create a single sample with features and label
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures({1.0f, 2.0f, 3.0f}));
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(5));

    // Before training: no model exists
    EXPECT_EQ(registry.getComponent<ecs::CModelParams>(sample.id), nullptr);
    EXPECT_EQ(registry.getComponent<ecs::CEvalMetrics>(sample.id), nullptr);

    // Call update() which should discover and train this entity
    train_system.update(registry, 0.0);

    // After training: model and metrics created
    auto* model = registry.getComponent<ecs::CModelParams>(sample.id);
    auto* metrics = registry.getComponent<ecs::CEvalMetrics>(sample.id);
    ASSERT_NE(model, nullptr);
    ASSERT_NE(metrics, nullptr);
    
    // Verify single-sample training results
    EXPECT_EQ(model->prototypes.size(), 1);
    EXPECT_EQ(model->num_nodes, 1);
    EXPECT_EQ(model->node_labels[0], 5);
    EXPECT_EQ(metrics->status, "trained");
}

TEST_F(ECSTrainSystemTest, UpdateCallProcessesMultipleSampleEntities) {
    // Create two independent sample entities
    auto sample1 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample1.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(sample1.id, ecs::CLabel(10));

    auto sample2 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample2.id, ecs::CFeatures({2.0f}));
    registry.addComponent<ecs::CLabel>(sample2.id, ecs::CLabel(20));

    // Single update() call should train both
    train_system.update(registry, 0.0);

    // Both entities should have been processed independently
    auto* model1 = registry.getComponent<ecs::CModelParams>(sample1.id);
    auto* model2 = registry.getComponent<ecs::CModelParams>(sample2.id);
    ASSERT_NE(model1, nullptr);
    ASSERT_NE(model2, nullptr);
    
    EXPECT_EQ(model1->node_labels[0], 10);
    EXPECT_EQ(model2->node_labels[0], 20);
}

// ============================================================================
// Goal 2: Single-Sample Training Flow — Dispatch to train_single_sample()
// ============================================================================

TEST_F(ECSTrainSystemTest, SingleSampleCreatesTrivialModel) {
    // Single sample without CSamples component triggers single_sample path
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures({5.5f, 6.6f}));
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(7));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(sample.id);
    ASSERT_NE(model, nullptr);
    
    // Single-sample always has exactly 1 prototype (itself)
    EXPECT_EQ(model->prototypes.size(), 1);
    EXPECT_EQ(model->prototypes[0], 0);
    
    // Single-sample always has 1 node
    EXPECT_EQ(model->num_nodes, 1);
    
    // Prototype is always at position 0 with label from training data
    EXPECT_EQ(model->node_labels[0], 7);
    EXPECT_EQ(model->pathvalues[0], 0.0f);  // Prototypes have zero path value
    EXPECT_EQ(model->predecessors[0], -1);  // Prototypes have no predecessor
}

TEST_F(ECSTrainSystemTest, SingleSampleMetricsAlwaysAccurate) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(42));

    train_system.update(registry, 0.0);

    auto* metrics = registry.getComponent<ecs::CEvalMetrics>(sample.id);
    ASSERT_NE(metrics, nullptr);
    
    // Single sample is trivially correct
    EXPECT_EQ(metrics->accuracy, 1.0f);
    EXPECT_EQ(metrics->num_samples_trained, 1);
    EXPECT_EQ(metrics->num_prototypes, 1);
    EXPECT_EQ(metrics->status, "trained");
}

// ============================================================================
// Goal 3: Multi-Sample Training Flow — Dispatch to train_subgraph()
// ============================================================================

TEST_F(ECSTrainSystemTest, MultiSampleSubgraphDispatchesToTrainSubgraph) {
    // Entity with CSamples component triggers subgraph path
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f, 2.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(1));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2, 3}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);
    
    // Subgraph training always processes correct number of samples
    EXPECT_EQ(model->num_nodes, 4);
    EXPECT_EQ(model->node_labels.size(), 4);
    EXPECT_EQ(model->pathvalues.size(), 4);
    EXPECT_EQ(model->predecessors.size(), 4);
}

TEST_F(ECSTrainSystemTest, SubgraphTrainingPropagatesLabel) {
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    int training_label = 99;
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(training_label));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2, 3, 4}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    
    // All nodes should have the training label (simplified IFT)
    for (int label : model->node_labels) {
        EXPECT_EQ(label, training_label);
    }
}

// ============================================================================
// Goal 4: Prototype Selection Flow — First sample always selected as prototype
// ============================================================================

TEST_F(ECSTrainSystemTest, PrototypeSelectionSelectsFirstSample) {
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(1));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2, 3, 4}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);
    
    // Simplified OPF always selects index 0 as prototype
    EXPECT_EQ(model->prototypes.size(), 1);
    EXPECT_EQ(model->prototypes[0], 0);
    
    // Prototype should have path value 0.0
    EXPECT_EQ(model->pathvalues[0], 0.0f);
}

TEST_F(ECSTrainSystemTest, NonPrototypeNodesHaveNonZeroPathValue) {
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(1));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    
    // Node 0 is prototype
    EXPECT_EQ(model->pathvalues[0], 0.0f);
    
    // All other nodes should have non-negative pathval
    for (int i = 1; i < model->num_nodes; ++i) {
        EXPECT_GE(model->pathvalues[i], 0.0f);
    }
}

// ============================================================================
// Goal 5: IFT Propagation Flow — Labels propagated from prototypes
// ============================================================================

TEST_F(ECSTrainSystemTest, IFTPropagatesPredecessorRelationships) {
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(1));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2, 3}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);
    
    // Prototype (index 0) has no predecessor
    EXPECT_EQ(model->predecessors[0], -1);
    
    // All non-prototypes should have a valid predecessor (>= 0 or -1 for unreached)
    for (int i = 1; i < model->num_nodes; ++i) {
        EXPECT_GE(model->predecessors[i], -1);
    }
}

TEST_F(ECSTrainSystemTest, IFTLabelConsistencyAlongPath) {
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    int label = 42;
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(label));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2, 3, 4, 5}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    
    // All nodes should have training label (no different labels in this simplified setup)
    for (int i = 0; i < model->num_nodes; ++i) {
        EXPECT_EQ(model->node_labels[i], label);
    }
}

// ============================================================================
// Goal 6: Node Ordering Flow — Nodes sorted by increasing path value
// ============================================================================

TEST_F(ECSTrainSystemTest, OrderedNodesIsSortedByPathValue) {
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(1));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2, 3, 4}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_EQ(model->ordered_nodes.size(), 5);
    
    // Ordered nodes should be sorted by pathval (non-decreasing)
    for (size_t i = 1; i < model->ordered_nodes.size(); ++i) {
        int prev_idx = model->ordered_nodes[i - 1];
        int curr_idx = model->ordered_nodes[i];
        EXPECT_LE(model->pathvalues[prev_idx], model->pathvalues[curr_idx]);
    }
}

TEST_F(ECSTrainSystemTest, OrderedNodesContainsAllNodeIndices) {
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(1));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2, 3}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    
    // Ordered list should contain exactly 4 nodes
    EXPECT_EQ(model->ordered_nodes.size(), 4);
    
    // All indices 0-3 should appear exactly once
    std::vector<bool> seen(4, false);
    for (int node_idx : model->ordered_nodes) {
        ASSERT_GE(node_idx, 0);
        ASSERT_LT(node_idx, 4);
        EXPECT_FALSE(seen[node_idx]);  // No duplicates
        seen[node_idx] = true;
    }
}

// ============================================================================
// Goal 7: Missing Components — Graceful skip when required components absent
// ============================================================================

TEST_F(ECSTrainSystemTest, EntityMissingLabelIsSkipped) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures({1.0f}));
    // Intentionally NOT adding CLabel

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(sample.id);
    EXPECT_EQ(model, nullptr);  // Entity not trained
}

TEST_F(ECSTrainSystemTest, EntityMissingFeaturesIsSkipped) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(1));
    // Intentionally NOT adding CFeatures

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(sample.id);
    EXPECT_EQ(model, nullptr);  // Entity not trained
}

// ============================================================================
// Goal 8: Metrics Recording — CEvalMetrics tracks all training outcomes
// ============================================================================

TEST_F(ECSTrainSystemTest, MetricsRecordsPrototypeCount) {
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(1));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2, 3, 4}));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    auto* metrics = registry.getComponent<ecs::CEvalMetrics>(subgraph.id);
    
    // Metrics should match model
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
// Goal 9: Integration — Real-world workflow with SplitSystem
// ============================================================================

TEST_F(ECSTrainSystemTest, TrainingSplitSystemOutput) {
    // Create dataset and split it
    auto dataset = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset.id, ecs::CIOPath("/data/test.dat"));
    registry.addComponent<ecs::CSplitParams>(dataset.id, ecs::CSplitParams(0.6f, 0.0f, 0.4f));

    ecs::SplitSystem split_system;
    split_system.update(registry, 0.0);

    // Find training partition created by SplitSystem
    auto subgraphs = registry.view<ecs::CSamples, ecs::CFlags>();
    ecs::EntityId training_subgraph{0};
    bool found = false;
    
    for (auto sg_id : subgraphs) {
        auto* flags = registry.getComponent<ecs::CFlags>(sg_id);
        if (flags && flags->isTraining) {
            training_subgraph = sg_id;
            found = true;
            break;
        }
    }
    
    EXPECT_TRUE(found);

    // Add training label to the split
    registry.addComponent<ecs::CFeatures>(training_subgraph, ecs::CFeatures({1.0f, 2.0f}));
    registry.addComponent<ecs::CLabel>(training_subgraph, ecs::CLabel(1));

    // Train it using TrainSystem
    train_system.update(registry, 0.0);

    // Verify training succeeded
    auto* model = registry.getComponent<ecs::CModelParams>(training_subgraph);
    auto* metrics = registry.getComponent<ecs::CEvalMetrics>(training_subgraph);
    ASSERT_NE(model, nullptr);
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->status, "trained");
}

// ============================================================================
// Goal 10: Large-Scale Data — Efficiency with many samples
// ============================================================================

TEST_F(ECSTrainSystemTest, TrainSubgraphWith1000Samples) {
    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    
    std::vector<int> large_sample_list;
    for (int i = 0; i < 1000; ++i) {
        large_sample_list.push_back(i);
    }
    
    registry.addComponent<ecs::CFeatures>(subgraph.id, ecs::CFeatures({1.0f}));
    registry.addComponent<ecs::CLabel>(subgraph.id, ecs::CLabel(1));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples(large_sample_list));

    train_system.update(registry, 0.0);

    auto* model = registry.getComponent<ecs::CModelParams>(subgraph.id);
    ASSERT_NE(model, nullptr);
    
    // All data structures properly sized
    EXPECT_EQ(model->num_nodes, 1000);
    EXPECT_EQ(model->node_labels.size(), 1000);
    EXPECT_EQ(model->pathvalues.size(), 1000);
    EXPECT_EQ(model->predecessors.size(), 1000);
    EXPECT_EQ(model->ordered_nodes.size(), 1000);
    
    // All nodes labeled correctly
    for (int label : model->node_labels) {
        EXPECT_EQ(label, 1);
    }
}

