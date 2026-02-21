#include <gtest/gtest.h>
#include "ecs/Entity.h"
#include "ecs/EntityRegistry.h"
#include "ecs/Components.hpp"

class ECSComponentsTest : public ::testing::Test {
protected:
    ecs::EntityRegistry registry;
};

TEST_F(ECSComponentsTest, CreateEntity) {
    auto entity = registry.create(ecs::EntityType::Sample);
    EXPECT_NE(entity.id, 0);
    EXPECT_EQ(entity.type, ecs::EntityType::Sample);
}

TEST_F(ECSComponentsTest, AddCFeatures) {
    auto entity = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(entity.id, ecs::CFeatures({1.0f, 2.0f, 3.0f}));
    
    EXPECT_TRUE(registry.hasComponent<ecs::CFeatures>(entity.id));
    auto *features = registry.getComponent<ecs::CFeatures>(entity.id);
    ASSERT_NE(features, nullptr);
    EXPECT_EQ(features->values.size(), 3);
    EXPECT_FLOAT_EQ(features->values[0], 1.0f);
    EXPECT_FLOAT_EQ(features->values[1], 2.0f);
    EXPECT_FLOAT_EQ(features->values[2], 3.0f);
}

TEST_F(ECSComponentsTest, AddCLabel) {
    auto entity = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(entity.id, ecs::CLabel(42));
    
    EXPECT_TRUE(registry.hasComponent<ecs::CLabel>(entity.id));
    auto *label = registry.getComponent<ecs::CLabel>(entity.id);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->label, 42);
}

TEST_F(ECSComponentsTest, MultipleComponents) {
    auto entity = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(entity.id, ecs::CFeatures({1.0f, 2.0f}));
    registry.addComponent<ecs::CLabel>(entity.id, ecs::CLabel(10));
    registry.addComponent<ecs::CIOPath>(entity.id, ecs::CIOPath{"/path/to/data.dat"});
    
    EXPECT_TRUE(registry.hasComponent<ecs::CFeatures>(entity.id));
    EXPECT_TRUE(registry.hasComponent<ecs::CLabel>(entity.id));
    EXPECT_TRUE(registry.hasComponent<ecs::CIOPath>(entity.id));
}

TEST_F(ECSComponentsTest, ViewWithMultipleComponents) {
    // Create entity with both features and label
    auto entity1 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(entity1.id, ecs::CFeatures({1.0f, 2.0f}));
    registry.addComponent<ecs::CLabel>(entity1.id, ecs::CLabel(1));
    
    // Create entity with only features
    auto entity2 = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CFeatures>(entity2.id, ecs::CFeatures({3.0f, 4.0f}));
    
    // Query for entities with both features and label
    auto results = registry.view<ecs::CFeatures, ecs::CLabel>();
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], entity1.id);
}

TEST_F(ECSComponentsTest, RemoveComponent) {
    auto entity = registry.create(ecs::EntityType::Sample);
    registry.addComponent<ecs::CLabel>(entity.id, ecs::CLabel(42));
    
    EXPECT_TRUE(registry.hasComponent<ecs::CLabel>(entity.id));
    registry.removeComponent<ecs::CLabel>(entity.id);
    EXPECT_FALSE(registry.hasComponent<ecs::CLabel>(entity.id));
}

TEST_F(ECSComponentsTest, CFeaturesCopyConstructor) {
    std::vector<float> values{1.5f, 2.5f, 3.5f};
    ecs::CFeatures comp(values);
    
    EXPECT_EQ(comp.values.size(), 3);
    EXPECT_FLOAT_EQ(comp.values[0], 1.5f);
}

TEST_F(ECSComponentsTest, CFeaturesDefaultConstructor) {
    ecs::CFeatures comp;
    EXPECT_EQ(comp.values.size(), 0);
}

TEST_F(ECSComponentsTest, CLabelConstructor) {
    ecs::CLabel comp(99);
    EXPECT_EQ(comp.label, 99);
}

TEST_F(ECSComponentsTest, CLabelDefaultConstructor) {
    ecs::CLabel comp;
    EXPECT_EQ(comp.label, -1);
}
