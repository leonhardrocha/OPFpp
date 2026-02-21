#include <gtest/gtest.h>
#include "ecs/Entity.h"
#include "ecs/EntityRegistry.h"
#include "ecs/Components.hpp"
#include "ecs/AccuracySystem.hpp"

class ECSAccuracySystemTest : public ::testing::Test {
protected:
    ecs::EntityRegistry registry;
    ecs::AccuracySystem accuracy_system;

    std::vector<int> toIntVector(const std::vector<ecs::EntityId>& ids) {
        std::vector<int> result;
        for (auto id : ids) {
            result.push_back(static_cast<int>(id));
        }
        return result;
    }
};

// ============================================================================
// Goal 1: Single-entity accuracy
// ============================================================================

TEST_F(ECSAccuracySystemTest, SingleEntityCorrectPrediction) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(1));
    registry.addComponent<ecs::CTrueLabel>(sample.id, ecs::CTrueLabel(1));

    accuracy_system.update(registry, 0.0);

    auto *metrics = registry.getComponent<ecs::CEvalMetrics>(sample.id);
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->correct_classifications, 1);
    EXPECT_EQ(metrics->total_classifications, 1);
    EXPECT_EQ(metrics->accuracy, 1.0f);
    EXPECT_EQ(metrics->status, "evaluated");
}

TEST_F(ECSAccuracySystemTest, SingleEntityIncorrectPrediction) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(0));
    registry.addComponent<ecs::CTrueLabel>(sample.id, ecs::CTrueLabel(1));

    accuracy_system.update(registry, 0.0);

    auto *metrics = registry.getComponent<ecs::CEvalMetrics>(sample.id);
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->correct_classifications, 0);
    EXPECT_EQ(metrics->total_classifications, 1);
    EXPECT_EQ(metrics->accuracy, 0.0f);
    EXPECT_EQ(metrics->status, "evaluated");
}

TEST_F(ECSAccuracySystemTest, SingleEntityMissingLabelsIsSkipped) {
    auto sample = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CTrueLabel>(sample.id, ecs::CTrueLabel(1));

    accuracy_system.update(registry, 0.0);

    auto *metrics = registry.getComponent<ecs::CEvalMetrics>(sample.id);
    EXPECT_EQ(metrics, nullptr);
}

// ============================================================================
// Goal 2: Subgraph accuracy
// ============================================================================

TEST_F(ECSAccuracySystemTest, SubgraphAccuracyComputedFromSamples) {
    std::vector<ecs::EntityId> sample_ids;

    // 3 samples: 2 correct, 1 incorrect
    auto s1 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(s1.id, ecs::CLabel(1));
    registry.addComponent<ecs::CTrueLabel>(s1.id, ecs::CTrueLabel(1));
    sample_ids.push_back(s1.id);

    auto s2 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(s2.id, ecs::CLabel(2));
    registry.addComponent<ecs::CTrueLabel>(s2.id, ecs::CTrueLabel(2));
    sample_ids.push_back(s2.id);

    auto s3 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(s3.id, ecs::CLabel(0));
    registry.addComponent<ecs::CTrueLabel>(s3.id, ecs::CTrueLabel(1));
    sample_ids.push_back(s3.id);

    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    accuracy_system.update(registry, 0.0);

    auto *metrics = registry.getComponent<ecs::CEvalMetrics>(subgraph.id);
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->correct_classifications, 2);
    EXPECT_EQ(metrics->total_classifications, 3);
    EXPECT_NEAR(metrics->accuracy, 2.0f / 3.0f, 1e-6f);
    EXPECT_EQ(metrics->status, "evaluated");
}

TEST_F(ECSAccuracySystemTest, SubgraphIgnoresSamplesMissingLabels) {
    std::vector<ecs::EntityId> sample_ids;

    auto s1 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(s1.id, ecs::CLabel(1));
    registry.addComponent<ecs::CTrueLabel>(s1.id, ecs::CTrueLabel(1));
    sample_ids.push_back(s1.id);

    auto s2 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CTrueLabel>(s2.id, ecs::CTrueLabel(1));
    sample_ids.push_back(s2.id);

    auto s3 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(s3.id, ecs::CLabel(1));
    sample_ids.push_back(s3.id);

    auto subgraph = registry.create(ecs::EntityType::Subgraph);
    ecs::CSamples samples;
    samples.indices = toIntVector(sample_ids);
    registry.addComponent<ecs::CSamples>(subgraph.id, samples);

    accuracy_system.update(registry, 0.0);

    auto *metrics = registry.getComponent<ecs::CEvalMetrics>(subgraph.id);
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->correct_classifications, 1);
    EXPECT_EQ(metrics->total_classifications, 1);
    EXPECT_EQ(metrics->accuracy, 1.0f);
    EXPECT_EQ(metrics->status, "evaluated");
}
