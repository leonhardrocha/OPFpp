#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include "ecs/Entity.h"
#include "ecs/EntityRegistry.h"
#include "ecs/Components.hpp"
#include "ecs/IOSystem.hpp"
#include "ecs/SplitSystem.hpp"

class ECSIOSystemTest : public ::testing::Test {
protected:
    ecs::EntityRegistry registry;
    ecs::IOSystem io_system;
    ecs::SplitSystem split_system;

    void TearDown() override {
        // Clean up test files
        std::remove("test_entity.txt");
        std::remove("test_entities.txt");
        std::remove("test_sample.txt");
        std::remove("test_workflow.txt");
        std::remove("test_split_result.txt");
    }
};

// ============================================================================
// Goal 1: Round-trip Fidelity — Save and load with complete data preservation
// ============================================================================

TEST_F(ECSIOSystemTest, RoundTripPreservesAllComponentData) {
    // Create a complex entity with many components
    auto original = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(original.id, ecs::CFeatures({1.1f, 2.2f, 3.3f, 4.4f, 5.5f}));
    registry.addComponent<ecs::CLabel>(original.id, ecs::CLabel(17));
    registry.addComponent<ecs::CIOPath>(original.id, ecs::CIOPath("/complex/nested/path/to/sample_123.dat"));
    registry.addComponent<ecs::CFlags>(original.id, ecs::CFlags(true, false, true));

    // Save
    io_system.saveEntity(registry, original.id, "test_sample.txt");

    // Load into new entity
    auto loaded = registry.create(ecs::EntityType::Sample);
    io_system.loadEntity(registry, loaded.id, "test_sample.txt");

    // Verify ALL components and ALL values
    auto *orig_feat = registry.getComponent<ecs::CFeatures>(original.id);
    auto *load_feat = registry.getComponent<ecs::CFeatures>(loaded.id);
    ASSERT_NE(load_feat, nullptr);
    EXPECT_EQ(load_feat->values.size(), orig_feat->values.size());
    for (size_t i = 0; i < load_feat->values.size(); ++i) {
        EXPECT_FLOAT_EQ(load_feat->values[i], orig_feat->values[i]);
    }

    auto *orig_label = registry.getComponent<ecs::CLabel>(original.id);
    auto *load_label = registry.getComponent<ecs::CLabel>(loaded.id);
    EXPECT_NE(load_label, nullptr);
    EXPECT_EQ(load_label->label, orig_label->label);

    auto *orig_path = registry.getComponent<ecs::CIOPath>(original.id);
    auto *load_path = registry.getComponent<ecs::CIOPath>(loaded.id);
    EXPECT_NE(load_path, nullptr);
    EXPECT_EQ(load_path->path, orig_path->path);

    auto *orig_flags = registry.getComponent<ecs::CFlags>(original.id);
    auto *load_flags = registry.getComponent<ecs::CFlags>(loaded.id);
    EXPECT_NE(load_flags, nullptr);
    EXPECT_EQ(load_flags->isTraining, orig_flags->isTraining);
    EXPECT_EQ(load_flags->isEvaluation, orig_flags->isEvaluation);
    EXPECT_EQ(load_flags->isTesting, orig_flags->isTesting);
}

TEST_F(ECSIOSystemTest, RoundTripWithEdgeCaseValues) {
    // Test boundary/edge-case numeric values
    auto original = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(original.id, ecs::CFeatures({0.0f, -99.5f, 1e6f}));
    registry.addComponent<ecs::CLabel>(original.id, ecs::CLabel(-1));  // -1 is "no label" sentinel

    io_system.saveEntity(registry, original.id, "test_sample.txt");

    auto loaded = registry.create(ecs::EntityType::Sample);
    io_system.loadEntity(registry, loaded.id, "test_sample.txt");

    auto *load_feat = registry.getComponent<ecs::CFeatures>(loaded.id);
    EXPECT_FLOAT_EQ(load_feat->values[0], 0.0f);
    EXPECT_FLOAT_EQ(load_feat->values[1], -99.5f);
    EXPECT_FLOAT_EQ(load_feat->values[2], 1e6f);

    auto *load_label = registry.getComponent<ecs::CLabel>(loaded.id);
    EXPECT_EQ(load_label->label, -1);
}

// ============================================================================
// Goal 2: Realistic Workflow — Integration with SplitSystem output
// ============================================================================

TEST_F(ECSIOSystemTest, SaveAndRestoreDatasetSplitResults) {
    // Create a dataset and split it
    auto dataset = registry.create(ecs::EntityType::Dataset);
    registry.addComponent<ecs::CIOPath>(dataset.id, ecs::CIOPath("/data/boat.dat"));
    registry.addComponent<ecs::CSplitParams>(dataset.id, ecs::CSplitParams(0.6f, 0.2f, 0.2f));

    split_system.update(registry, 0.0);

    // Collect all subgraph entities created by SplitSystem
    auto subgraphs = registry.view<ecs::CSamples, ecs::CFlags>();
    EXPECT_GE(subgraphs.size(), 2);  // At least train + test
    
    int training_count = 0, testing_count = 0;
    for (auto subgraph_id : subgraphs) {
        auto *flags = registry.getComponent<ecs::CFlags>(subgraph_id);
        if (flags->isTraining) training_count++;
        if (flags->isTesting) testing_count++;
    }
    EXPECT_GE(training_count, 1);
    EXPECT_GE(testing_count, 1);

    // Save all subgraph entities
    io_system.saveEntities(registry, subgraphs, "test_split_result.txt");

    // Verify file was created
    std::ifstream file("test_split_result.txt");
    EXPECT_TRUE(file.is_open());
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    EXPECT_GT(content.length(), 0);  // File has content
    EXPECT_NE(content.find("entity_count:"), std::string::npos);  // Has batch header

    // Create a fresh registry and load back each subgraph
    ecs::EntityRegistry fresh_registry;
    ecs::IOSystem fresh_io;
    
    int loaded_count = 0;
    for (auto original_id : subgraphs) {
        auto loaded = fresh_registry.create(ecs::EntityType::Subgraph);
        
        // Create temp file for single entity
        io_system.saveEntity(registry, original_id, "temp_subgraph.txt");
        bool load_success = fresh_io.loadEntity(fresh_registry, loaded.id, "temp_subgraph.txt");
        
        if (load_success) {
            // Verify the subgraph has its components
            auto *orig_samples = registry.getComponent<ecs::CSamples>(original_id);
            auto *loaded_samples = fresh_registry.getComponent<ecs::CSamples>(loaded.id);
            
            EXPECT_NE(loaded_samples, nullptr);
            if (loaded_samples && orig_samples) {
                EXPECT_EQ(loaded_samples->indices.size(), orig_samples->indices.size());
                loaded_count++;
            }
        }
        std::remove("temp_subgraph.txt");
    }
    
    EXPECT_GT(loaded_count, 0);  // At least one subgraph loaded successfully
}

// ============================================================================
// Goal 3: Entity State Updates — Load, modify, re-save
// ============================================================================

TEST_F(ECSIOSystemTest, LoadModifyReSaveEntity) {
    // Original entity
    auto original = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(original.id, ecs::CLabel(1));
    registry.addComponent<ecs::CFeatures>(original.id, ecs::CFeatures({0.1f, 0.2f}));

    io_system.saveEntity(registry, original.id, "test_entity.txt");

    // Simulate loading in a different context
    auto loaded = registry.create(ecs::EntityType::Sample);
    io_system.loadEntity(registry, loaded.id, "test_entity.txt");

    // Modify loaded entity (e.g., update flags after processing)
    registry.removeComponent<ecs::CLabel>(loaded.id);
    registry.addComponent<ecs::CLabel>(loaded.id, ecs::CLabel(2));  // Changed label
    registry.addComponent<ecs::CFlags>(loaded.id, ecs::CFlags(false, true, false));  // Mark as evaluated

    // Re-save modified entity
    io_system.saveEntity(registry, loaded.id, "test_entity.txt");

    // Load again and verify modifications persisted
    auto reloaded = registry.create(ecs::EntityType::Sample);
    io_system.loadEntity(registry, reloaded.id, "test_entity.txt");

    auto *label = registry.getComponent<ecs::CLabel>(reloaded.id);
    EXPECT_EQ(label->label, 2);  // Updated label

    auto *flags = registry.getComponent<ecs::CFlags>(reloaded.id);
    EXPECT_FALSE(flags->isTraining);
    EXPECT_TRUE(flags->isEvaluation);
    EXPECT_FALSE(flags->isTesting);
}

// ============================================================================
// Goal 4: Batch Entity Operations — Multiple entities, consistent handling
// ============================================================================

TEST_F(ECSIOSystemTest, BatchSaveMultipleEntitiesPreservesIdentity) {
    // Create diverse entities
    auto sample1 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(sample1.id, ecs::CLabel(10));
    registry.addComponent<ecs::CFeatures>(sample1.id, ecs::CFeatures({1.0f}));

    auto sample2 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(sample2.id, ecs::CLabel(20));
    registry.addComponent<ecs::CFeatures>(sample2.id, ecs::CFeatures({2.0f, 2.0f}));

    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    registry.addComponent<ecs::CFlags>(subgraph.id, ecs::CFlags(true, false, false));
    registry.addComponent<ecs::CSamples>(subgraph.id, ecs::CSamples({0, 1, 2}));

    std::vector<ecs::EntityId> ids{sample1.id, sample2.id, subgraph.id};
    io_system.saveEntities(registry, ids, "test_entities.txt");

    // Verify batch file structure
    std::ifstream file("test_entities.txt");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    // Check each entity is present with correct data
    EXPECT_NE(content.find("[entity:" + std::to_string(sample1.id) + "]"), std::string::npos);
    EXPECT_NE(content.find("[entity:" + std::to_string(sample2.id) + "]"), std::string::npos);
    EXPECT_NE(content.find("[entity:" + std::to_string(subgraph.id) + "]"), std::string::npos);
    EXPECT_NE(content.find("label: 10"), std::string::npos);
    EXPECT_NE(content.find("label: 20"), std::string::npos);
}

// ============================================================================
// Goal 5: Selective Component Handling — Only present components saved/loaded
// ============================================================================

TEST_F(ECSIOSystemTest, OnlyPopulatedComponentsSavedAndLoaded) {
    // Entity with only some components
    auto minimal = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(minimal.id, ecs::CLabel(99));
    // Intentionally NOT adding CFeatures, CIOPath, CFlags

    io_system.saveEntity(registry, minimal.id, "test_entity.txt");

    // Load and verify
    auto loaded = registry.create(ecs::EntityType::Sample);
    io_system.loadEntity(registry, loaded.id, "test_entity.txt");

    // Only CLabel should be present
    EXPECT_TRUE(registry.hasComponent<ecs::CLabel>(loaded.id));
    EXPECT_FALSE(registry.hasComponent<ecs::CFeatures>(loaded.id));
    EXPECT_FALSE(registry.hasComponent<ecs::CIOPath>(loaded.id));
    EXPECT_FALSE(registry.hasComponent<ecs::CFlags>(loaded.id));

    auto *label = registry.getComponent<ecs::CLabel>(loaded.id);
    EXPECT_EQ(label->label, 99);
}

// ============================================================================
// Goal 6: Error Handling — Graceful failure on missing/invalid files
// ============================================================================

TEST_F(ECSIOSystemTest, LoadFromNonexistentFileReturnsFailure) {
    auto entity = registry.create(ecs::EntityType::Sample);
    bool result = io_system.loadEntity(registry, entity.id, "does_not_exist_12345.txt");
    EXPECT_FALSE(result);
}

TEST_F(ECSIOSystemTest, SaveToValidDirectorySucceeds) {
    auto entity = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(entity.id, ecs::CLabel(42));

    bool saved = io_system.saveEntity(registry, entity.id, "test_entity.txt");
    EXPECT_TRUE(saved);

    std::ifstream file("test_entity.txt");
    EXPECT_TRUE(file.is_open());
    file.close();
}

// ============================================================================
// Goal 7: Large Entity Data — Handle entities with many samples/features
// ============================================================================

TEST_F(ECSIOSystemTest, SaveLoadLargeFeatureVector) {
    auto entity = registry.create(ecs::EntityType::Sample);
    
    // Create large feature vector (1000 floats)
    std::vector<float> large_features;
    for (int i = 0; i < 1000; ++i) {
        large_features.push_back(static_cast<float>(i) / 1000.0f);
    }
    registry.addComponent<ecs::CFeatures>(entity.id, ecs::CFeatures(large_features));

    io_system.saveEntity(registry, entity.id, "test_entity.txt");

    auto loaded = registry.create(ecs::EntityType::Sample);
    io_system.loadEntity(registry, loaded.id, "test_entity.txt");

    auto *features = registry.getComponent<ecs::CFeatures>(loaded.id);
    EXPECT_NE(features, nullptr);
    EXPECT_EQ(features->values.size(), 1000);
    
    // Spot check a few values
    EXPECT_FLOAT_EQ(features->values[0], 0.0f);
    EXPECT_FLOAT_EQ(features->values[500], 500.0f / 1000.0f);
    EXPECT_FLOAT_EQ(features->values[999], 999.0f / 1000.0f);
}

TEST_F(ECSIOSystemTest, SaveLoadLargeSampleList) {
    auto entity = registry.create(ecs::EntityType::Subgraph);
    
    // Create large sample index list (5000 indices)
    std::vector<int> large_samples;
    for (int i = 0; i < 5000; ++i) {
        large_samples.push_back(i);
    }
    registry.addComponent<ecs::CSamples>(entity.id, ecs::CSamples(large_samples));

    io_system.saveEntity(registry, entity.id, "test_entity.txt");

    auto loaded = registry.create(ecs::EntityType::Subgraph);
    io_system.loadEntity(registry, loaded.id, "test_entity.txt");

    auto *samples = registry.getComponent<ecs::CSamples>(loaded.id);
    EXPECT_NE(samples, nullptr);
    EXPECT_EQ(samples->indices.size(), 5000);
    EXPECT_EQ(samples->indices[0], 0);
    EXPECT_EQ(samples->indices[4999], 4999);
}

