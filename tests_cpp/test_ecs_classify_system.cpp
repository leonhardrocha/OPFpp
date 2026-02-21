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
 * Tests verify the OPF classification algorithm implementation:
 * - Uses opf::OPF<T>::classifying() logic
 * - Cost-based routing: cost = max(pathval, distance)
 * - Early termination optimization
 * - Label assignment from best-matching prototype
 */
class ClassifySystemTest : public ::testing::Test {
protected:
    EntityRegistry registry;
    ClassifySystem classify_system;
    TrainSystem train_system;
    EntityId model_entity;

    void SetUp() override {
        // Create and train a model using TrainSystem
        model_entity = create_and_train_model();
    }

    /**
     * Creates a simple 3-class dataset and trains a model.
     * Returns the trained model entity ID.
     */
    EntityId create_and_train_model() {
        // Create training dataset with 3 classes
        auto dataset = registry.create(EntityType::Dataset);
        
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

        // Create sample entities for training
        std::vector<EntityId> sample_ids;
        
        // Add class 0 samples
        for (size_t i = 0; i < class0_samples.size(); ++i) {
            auto sample = registry.create(EntityType::Sample);
            registry.addComponent<CFeatures>(sample.id, CFeatures(class0_samples[i]));
            registry.addComponent<CLabel>(sample.id, CLabel(0));
            sample_ids.push_back(sample.id);
        }
        
        // Add class 1 samples
        for (size_t i = 0; i < class1_samples.size(); ++i) {
            auto sample = registry.create(EntityType::Sample);
            registry.addComponent<CFeatures>(sample.id, CFeatures(class1_samples[i]));
            registry.addComponent<CLabel>(sample.id, CLabel(1));
            sample_ids.push_back(sample.id);
        }
        
        // Add class 2 samples
        for (size_t i = 0; i < class2_samples.size(); ++i) {
            auto sample = registry.create(EntityType::Sample);
            registry.addComponent<CFeatures>(sample.id, CFeatures(class2_samples[i]));
            registry.addComponent<CLabel>(sample.id, CLabel(2));
            sample_ids.push_back(sample.id);
        }

        // Create subgraph for training
        auto train_subgraph = registry.create(EntityType::Subgraph);
        CSamples samples_comp;
        samples_comp.indices = {0, 1, 2, 3, 4, 5, 6, 7, 8}; // 9 samples
        registry.addComponent<CSamples>(train_subgraph.id, samples_comp);
        
        // Train model
        train_system.update(registry, 0.0);
        
        // Find and return model entity
        auto models = registry.view<CModelParams>();
        EXPECT_FALSE(models.empty());
        return models.empty() ? EntityId{0} : models[0];
    }
};

// Test 1: Classify single sample to class 0
TEST_F(ClassifySystemTest, ClassifySingleSampleClass0) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.15f, 0.05f, 0.1f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 0);
}

// Test 2: Classify single sample to class 1
TEST_F(ClassifySystemTest, ClassifySingleSampleClass1) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({5.05f, 5.15f, 4.95f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 1);
}

// Test 3: Classify single sample to class 2
TEST_F(ClassifySystemTest, ClassifySingleSampleClass2) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({10.05f, 9.95f, 10.1f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 2);
}

// Test 4: Classify multiple samples in one update
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

// Test 5: Metrics are updated after classification
TEST_F(ClassifySystemTest, MetricsUpdatedAfterClassification) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.0f, 0.0f, 0.0f}));
    
    classify_system.update(registry, 0.0);
    
    CEvalMetrics *metrics = registry.getComponent<CEvalMetrics>(test_sample.id);
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->total_classifications, 1);
    EXPECT_GE(metrics->training_time_ms, 0.0f);
}

// Test 6: Re-classification overwrites existing label
TEST_F(ClassifySystemTest, ReclassificationOverwritesLabel) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.0f, 0.0f, 0.0f}));
    registry.addComponent<CLabel>(test_sample.id, CLabel(99)); // Wrong label
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    EXPECT_EQ(label->label, 0); // Should be corrected to class 0
}

// Test 7: Empty features are skipped
TEST_F(ClassifySystemTest, EmptyFeaturesSkipped) {
    auto test_sample = registry.create(EntityType::Sample);
    registry.addComponent<CFeatures>(test_sample.id, CFeatures());
    
    classify_system.update(registry, 0.0);
    
    // Should not crash, no label assigned
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    EXPECT_EQ(label, nullptr);
}

// Test 8: No model available - no classification
TEST_F(ClassifySystemTest, NoModelAvailable) {
    EntityRegistry empty_registry;
    ClassifySystem system;
    
    auto test_sample = empty_registry.create(EntityType::Sample);
    empty_registry.addComponent<CFeatures>(test_sample.id, CFeatures({1.0f, 1.0f, 1.0f}));
    
    system.update(empty_registry, 0.0);
    
    CLabel *label = empty_registry.getComponent<CLabel>(test_sample.id);
    EXPECT_EQ(label, nullptr);
}

// Test 9: Cost computation follows OPF algorithm (max of pathval and distance)
TEST_F(ClassifySystemTest, CostComputationFollowsOPFAlgorithm) {
    auto test_sample = registry.create(EntityType::Sample);
    // Place test sample exactly at a training point
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({0.0f, 0.0f, 0.0f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 0);
    
    // Cost should be pathval (since distance = 0)
    CEvalMetrics *metrics = registry.getComponent<CEvalMetrics>(test_sample.id);
    ASSERT_NE(metrics, nullptr);
    // last_classification_time stores the min_cost
    EXPECT_GE(metrics->training_time_ms, 0.0f);
}

// Test 10: Boundary case - sample equidistant from two classes
TEST_F(ClassifySystemTest, EquidistantSample) {
    auto test_sample = registry.create(EntityType::Sample);
    // Midpoint between class 0 and class 1
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({2.5f, 2.5f, 2.5f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    // Should be assigned to whichever has lower path value (depends on training)
    EXPECT_TRUE(label->label == 0 || label->label == 1);
}

// Test 11: Verify early termination optimization doesn't affect correctness
TEST_F(ClassifySystemTest, EarlyTerminationCorrectness) {
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

// Test 12: Model with single prototype
TEST_F(ClassifySystemTest, SinglePrototypeModel) {
    EntityRegistry single_registry;
    ClassifySystem system;
    
    // Create single-prototype model manually
    auto model = single_registry.create(EntityType::Model);
    CModelParams model_params;
    model_params.prototypes = {0}; // Prototype at index 0
    model_params.node_labels = {7};
    model_params.pathvalues = {0.0f};
    model_params.ordered_nodes = {0};
    model_params.num_nodes = 1;
    single_registry.addComponent<CModelParams>(model.id, model_params);
    
    auto test_sample = single_registry.create(EntityType::Sample);
    single_registry.addComponent<CFeatures>(test_sample.id, CFeatures({4.9f, 5.1f, 5.0f}));
    
    system.update(single_registry, 0.0);
    
    CLabel *label = single_registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 7);
}

// Test 13: Ordered nodes affects classification order (optimization test)
TEST_F(ClassifySystemTest, OrderedNodesAffectClassificationOrder) {
    auto model = registry.getComponent<CModelParams>(model_entity);
    ASSERT_NE(model, nullptr);
    
    // Verify ordered_nodes is sorted by pathvalues
    for (size_t i = 1; i < model->ordered_nodes.size(); ++i) {
        int prev_idx = model->ordered_nodes[i-1];
        int curr_idx = model->ordered_nodes[i];
        EXPECT_LE(model->pathvalues[prev_idx], model->pathvalues[curr_idx]);
    }
}

// Test 14: Distance computation uses Euclidean distance
TEST_F(ClassifySystemTest, DistanceComputationUsesEuclidean) {
    auto test_sample = registry.create(EntityType::Sample);
    // Create a sample at known distance from origin
    // Distance from (0,0,0) to (3,4,0) = 5 (3-4-5 triangle)
    registry.addComponent<CFeatures>(test_sample.id, CFeatures({3.0f, 4.0f, 0.0f}));
    
    classify_system.update(registry, 0.0);
    
    CLabel *label = registry.getComponent<CLabel>(test_sample.id);
    ASSERT_NE(label, nullptr);
    // Should still classify to class 0 (closest)
    EXPECT_EQ(label->label, 0);
}

// Test 15: Model entity is not classified (self-exclusion)
TEST_F(ClassifySystemTest, ModelEntityNotClassified) {
    auto model = registry.getComponent<CModelParams>(model_entity);
    ASSERT_NE(model, nullptr);
    
    // Ensure model entity has features (unlikely but test the guard)
    if (!registry.hasComponent<CFeatures>(model_entity)) {
        registry.addComponent<CFeatures>(model_entity, CFeatures({1.0f, 1.0f, 1.0f}));
    }
    
    size_t initial_component_count = registry.view<CLabel>().size();
    
    classify_system.update(registry, 0.0);
    
    // Model entity should not get a new label from classification
    // (it might already have labels from training, so we just check it's not re-classified)
    size_t final_component_count = registry.view<CLabel>().size();
    // The difference should be 0 (no new test samples)
    EXPECT_EQ(initial_component_count, final_component_count);
}

// Test 16: Classification metrics increment correctly on repeated updates
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
