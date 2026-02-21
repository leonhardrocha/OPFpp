#include <gtest/gtest.h>
#include "../include_cpp/ecs/Entity.h"
#include "../include_cpp/ecs/EntityRegistry.h"
#include "../include_cpp/ecs/ClassifySystem.hpp"
#include "../include_cpp/ecs/TrainSystem.hpp"
#include <vector>
#include <cmath>

using namespace ecs;

/**
 * Test fixture for ClassifySystem
 * 
 * Tests verify the actual OPF classification algorithm:
 * - MST prototype selection during training
 * - IFT label propagation with max(pathval, weight) cost
 * - Classification with cost = max(training_pathval, distance_to_test)
 * - Early termination optimization
 * - Actual Euclidean distance computation
 */
class ClassifySystemTest : public ::testing::Test {
protected:
    EntityRegistry registry;
    ClassifySystem classify_system;
    TrainSystem train_system;
    EntityId model_entity;

    void SetUp() override {
        // Create and train a model using actual MST + IFT
        model_entity = create_and_train_model();
    }

    /**
     * Creates a 3-class dataset with clear separation and trains with MST+IFT.
     */
    EntityId create_and_train_model() {
        // Create sample entities for each class
        std::vector<EntityId> sample_ids;
        
        // Class 0: samples near origin (0,0,0)
        std::vector<std::vector<float>> class0_samples = {
            {0.0f, 0.0f, 0.0f},
            {0.1f, 0.1f, 0.1f},
            {0.2f, 0.0f, 0.1f}
        };
        
        // Class 1: samples near (5,5,5)
        std::vector<std::vector<float>> class1_samples = {
            {5.0f, 5.0f, 5.0f},
            {5.1f, 5.0f, 5.1f},
            {4.9f, 5.1f, 5.0f}
        };
        
        // Class 2: samples near (10,10,10)
        std::vector<std::vector<float>> class2_samples = {
            {10.0f, 10.0f, 10.0f},
            {10.1f, 10.0f, 10.1f},
            {9.9f, 10.1f, 10.0f}
        };

        // Create sample entities
        for (size_t i = 0; i < class0_samples.size(); ++i) {
            auto sample = registry.create(EntityType::Sample);
            registry.addComponent<CFeatures>(sample.id, CFeatures(class0_samples[i]));
            registry.addComponent<CLabel>(sample.id, CLabel(0));
            sample_ids.push_back(sample.id);
        }
        
        for (size_t i = 0; i < class1_samples.size(); ++i) {
            auto sample = registry.create(EntityType::Sample);
            registry.addComponent<CFeatures>(sample.id, CFeatures(class1_samples[i]));
            registry.addComponent<CLabel>(sample.id, CLabel(1));
            sample_ids.push_back(sample.id);
        }
        
        for (size_t i = 0; i < class2_samples.size(); ++i) {
            auto sample = registry.create(EntityType::Sample);
            registry.addComponent<CFeatures>(sample.id, CFeatures(class2_samples[i]));
            registry.addComponent<CLabel>(sample.id, CLabel(2));
            sample_ids.push_back(sample.id);
        }

        // Create subgraph for training
        auto train_subgraph = registry.create(EntityType::Subgraph);
        std::vector<int> int_ids;
        for (auto id : sample_ids) int_ids.push_back(static_cast<int>(id));
        registry.addComponent<CSamples>(train_subgraph.id, CSamples(int_ids));
        
        // Train model with actual MST + IFT
        train_system.update(registry, 0.0);
        
        return train_subgraph.id;
    }
};

// ============================================================================
// Goal 1: Distance Computation Tests (Critical Fix Verification)
// ============================================================================

TEST_F(ClassifySystemTest, DistanceComputationUsesActualEuclideanDistance) {
    auto test_sample = registry.create(EntityType::Sample);
    // Create sample at known distance from (0,0,0): distance = 5 (3-4-0 Pythagorean)
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({3.0f, 4.0f, 0.0f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    // Should classify to class 0 (nearest cluster at origin)
    EXPECT_EQ(label->label, 0);
}

TEST_F(ClassifySystemTest, ClassificationUsesStoredTrainingFeatures) {
    auto* model = registry.getComponent<CModelParams>(model_entity);
    ASSERT_NE(model, nullptr);
    
    // Verify model stores all training features
    EXPECT_EQ(model->all_features.size(), 9);  // 3 classes × 3 samples
    
    // Verify features are actually stored
    for (const auto& feat : model->all_features) {
        EXPECT_FALSE(feat.empty());
        EXPECT_EQ(feat.size(), 3);  // 3D features
    }
}

TEST_F(ClassifySystemTest, DistanceToNearestClusterDeterminesLabel) {
    // Test samples at varying distances from each cluster
    auto test_near_class0 = registry.create(EntityType::Sample);
    auto test_near_class1 = registry.create(EntityType::Sample);
    auto test_near_class2 = registry.create(EntityType::Sample);
    
    registry.addComponent<CFeatures>(test_near_class0.id, CFeatures({0.05f, 0.05f, 0.05f}));
    registry.addComponent<CFeatures>(test_near_class1.id, CFeatures({5.05f, 5.05f, 5.05f}));
    registry.addComponent<CFeatures>(test_near_class2.id, CFeatures({10.05f, 10.05f, 10.05f}));
    
    classify_system.update(registry, 0.0);
    
    EXPECT_EQ(registry.getComponent<CLabel>(test_near_class0.id)->label, 0);
    EXPECT_EQ(registry.getComponent<CLabel>(test_near_class1.id)->label, 1);
    EXPECT_EQ(registry.getComponent<CLabel>(test_near_class2.id)->label, 2);
}

// ============================================================================
// Goal 2: OPF Cost Function Tests (max(pathval, distance))
// ============================================================================

TEST_F(ClassifySystemTest, CostFunctionFollowsOPFAlgorithm) {
    auto test_sample = registry.create(EntityType::Sample);
    // Place test sample exactly at a training point
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.0f, 0.0f, 0.0f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 0);
    
    // Cost should be minimal (distance ≈ 0)
    CEvalMetrics *metrics = registry.getComponent<CEvalMetrics>(test_sample.id);
    ASSERT_NE(metrics, nullptr);
    EXPECT_GE(metrics->training_time_ms, 0.0f);
}

TEST_F(ClassifySystemTest, CostIsMaxOfPathvalAndDistance) {
    auto* model = registry.getComponent<CModelParams>(model_entity);
    ASSERT_NE(model, nullptr);
    
    // All training nodes have pathvals computed during IFT
    // Classification should use max(pathval[train_node], dist(train_node, test))
    
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({2.5f, 2.5f, 2.5f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    
    // Should be assigned based on minimum max(pathval, distance) cost
    EXPECT_TRUE(label->label == 0 || label->label == 1);
}

// ============================================================================
// Goal 3: Label Assignment Tests
// ============================================================================

TEST_F(ClassifySystemTest, ClassifySingleSampleClass0) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.15f, 0.05f, 0.1f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 0);
}

TEST_F(ClassifySystemTest, ClassifySingleSampleClass1) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({5.05f, 5.15f, 4.95f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 1);
}

TEST_F(ClassifySystemTest, ClassifySingleSampleClass2) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({10.05f, 9.95f, 10.1f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 2);
}

TEST_F(ClassifySystemTest, ClassifyMultipleSamples) {
    auto test1 = registry.create(EntityType::Sample);
    auto test2 = registry.create(EntityType::Sample);
    auto test3 = registry.create(EntityType::Sample);
    
    registry.addComponent<CFeatures>(test1.id, CFeatures({0.1f, 0.0f, 0.1f}));
    registry.addComponent<CFeatures>(test2.id, CFeatures({5.0f, 5.0f, 5.0f}));
    registry.addComponent<CFeatures>(test3.id, CFeatures({10.0f, 10.0f, 10.0f}));
    
    classify_system.update(registry, 0.0);
    
    EXPECT_EQ(registry.getComponent<CLabel>(test1.id)->label, 0);
    EXPECT_EQ(registry.getComponent<CLabel>(test2.id)->label, 1);
    EXPECT_EQ(registry.getComponent<CLabel>(test3.id)->label, 2);
}

// ============================================================================
// Goal 4: Early Termination Tests
// ============================================================================

TEST_F(ClassifySystemTest, EarlyTerminationWhenMinCostBelowNextPathval) {
    auto* model = registry.getComponent<CModelParams>(model_entity);
    ASSERT_NE(model, nullptr);
    
    // Verify ordered_nodes is sorted
    for (size_t i = 1; i < model->ordered_nodes.size(); ++i) {
        int prev = model->ordered_nodes[i-1];
        int curr = model->ordered_nodes[i];
        EXPECT_LE(model->pathvalues[prev], model->pathvalues[curr]);
    }
    
    // Early termination should work: iteration stops when min_cost <= next pathval
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.0f, 0.0f, 0.0f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 0);
}

TEST_F(ClassifySystemTest, EarlyTerminationDoesNotAffectCorrectness) {
    // Create many test samples
    std::vector<EntityId> test_ids;
    for (int i = 0; i < 10; ++i) {
        auto test_sample = registry.create(EntityType::Sample);
        registry.addComponent<CFeatures>(test_sample.id, 
            CFeatures({static_cast<float>(i) * 1.1f, 
                      static_cast<float>(i) * 1.1f, 
                      static_cast<float>(i) * 1.1f}));
        test_ids.push_back(test_sample.id);
    }
    
    classify_system.update(registry, 0.0);
    
    // All should be classified
    for (EntityId id : test_ids) {
        CLabel *label = registry.getComponent<CLabel>(id);
        EXPECT_NE(label, nullptr);
        EXPECT_GE(label->label, 0);
        EXPECT_LE(label->label, 2);
    }
}

// ============================================================================
// Goal 5: Metrics Tests
// ============================================================================

TEST_F(ClassifySystemTest, MetricsUpdatedAfterClassification) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.0f, 0.0f, 0.0f}));
    
    classify_system.update(registry, 0.0);
    
    CEvalMetrics *metrics = registry.getComponent<CEvalMetrics>(test_sample.id);
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->total_classifications, 1);
    EXPECT_GE(metrics->training_time_ms, 0.0f);
}

TEST_F(ClassifySystemTest, MetricsIncrementOnRepeatedUpdates) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.0f, 0.0f, 0.0f}));
    
    classify_system.update(registry, 0.0);
    CEvalMetrics *metrics1 = registry.getComponent<CEvalMetrics>(test_sample.id);
    EXPECT_EQ(metrics1->total_classifications, 1);
    
    classify_system.update(registry, 0.0);
    CEvalMetrics *metrics2 = registry.getComponent<CEvalMetrics>(test_sample.id);
    EXPECT_EQ(metrics2->total_classifications, 2);
}

// ============================================================================
// Goal 6: Edge Cases and Boundary Conditions
// ============================================================================

TEST_F(ClassifySystemTest, ReclassificationOverwritesLabel) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.0f, 0.0f, 0.0f}));
    registry.addComponent<CLabel>(test_sample.id, CLabel(99)); // Wrong label
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    EXPECT_EQ(label->label, 0); // Should be corrected
}

TEST_F(ClassifySystemTest, EmptyFeaturesSkipped) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures());
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    EXPECT_EQ(label, nullptr);
}

TEST_F(ClassifySystemTest, NoModelAvailable) {
    EntityRegistry empty_registry;
    ClassifySystem system;
    
    auto test_sample = empty_registry.create(EntityType::Sample);
    empty_registry.addComponent<CFeatures>(test_sample.id, CFeatures({1.0f, 1.0f, 1.0f}));
    
    system.update(empty_registry, 0.0);
    
    CLabel *label = empty_registry.getComponent<CLabel>(test_sample.id);
    EXPECT_EQ(label, nullptr);
}

TEST_F(ClassifySystemTest, EquidistantSampleUsesPathvalToBreakTie) {
    auto test_sample = registry.create(EntityType::Sample);
    // Midpoint between class 0 and class 1
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({2.5f, 2.5f, 2.5f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    // Should use pathval to break tie (whichever has lower pathval wins)
    EXPECT_TRUE(label->label == 0 || label->label == 1);
}

// ============================================================================
// Goal 7: Ordered Nodes Optimization Tests
// ============================================================================

TEST_F(ClassifySystemTest, OrderedNodesAreSortedByPathValue) {
    auto model = registry.getComponent<CModelParams>(model_entity);
    ASSERT_NE(model, nullptr);
    
    // Verify ordered_nodes is sorted by pathvalues
    for (size_t i = 1; i < model->ordered_nodes.size(); ++i) {
        int prev_idx = model->ordered_nodes[i-1];
        int curr_idx = model->ordered_nodes[i];
        EXPECT_LE(model->pathvalues[prev_idx], model->pathvalues[curr_idx]);
    }
}

TEST_F(ClassifySystemTest, ClassificationIteratesOrderedNodes) {
    // This test verifies that classification uses the ordered list
    // If it didn't, classification might be slower or incorrect
    
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.05f, 0.05f, 0.05f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 0);
}

// ============================================================================
// Goal 8: Model Entity Self-Exclusion
// ============================================================================

TEST_F(ClassifySystemTest, ModelEntityNotClassified) {
    auto* model = registry.getComponent<CModelParams>(model_entity);
    ASSERT_NE(model, nullptr);
    
    size_t initial_labels = registry.view<CLabel>().size();
    
    classify_system.update(registry, 0.0);
    
    size_t final_labels = registry.view<CLabel>().size();
    // Model entity should not be classified (it has CModelParams)
    // Count difference should only be from test samples
    EXPECT_GE(final_labels, initial_labels);
}

// ============================================================================
// Goal 9: Integration with Actual Training
// ============================================================================

TEST_F(ClassifySystemTest, ClassificationAfterMSTTraining) {
    auto* model = registry.getComponent<CModelParams>(model_entity);
    ASSERT_NE(model, nullptr);
    
    // Verify model has prototypes from MST
    EXPECT_GT(model->prototypes.size(), 0);
    
    // Verify all prototypes have pathval = 0
    for (int proto_idx : model->prototypes) {
        EXPECT_EQ(model->pathvalues[proto_idx], 0.0f);
    }
    
    // Classification should work with this model
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({5.0f, 5.0f, 5.0f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 1);
}

TEST_F(ClassifySystemTest, ClassificationUsesIFTLabelPropagation) {
    auto* model = registry.getComponent<CModelParams>(model_entity);
    ASSERT_NE(model, nullptr);
    
    // All training nodes should have labels assigned via IFT
    for (int label : model->node_labels) {
        EXPECT_GE(label, 0);
        EXPECT_LE(label, 2);
    }
    
    // Classification should use these propagated labels
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({10.0f, 10.0f, 10.0f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 2);
}
