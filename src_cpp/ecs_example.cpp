#include <iostream>
#include "../include_cpp/ecs/EntityRegistry.h"
#include "../include_cpp/ecs/Components.hpp"

int main() {
    ecs::EntityRegistry registry;

    // Create a sample entity
    auto sample = registry.create(ecs::EntityType::Sample);

    // Attach components
    registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures{std::vector<float>{1.0f, 2.0f, 3.0f}});
    registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel{42});

    // Query entities with both features and label
    auto list = registry.view<ecs::CFeatures, ecs::CLabel>();
    std::cout << "Entities with features+label: " << list.size() << std::endl;
    for (auto id : list) {
        auto f = registry.getComponent<ecs::CFeatures>(id);
        auto l = registry.getComponent<ecs::CLabel>(id);
        if (f && l) std::cout << "Entity " << id << " label=" << l->label << " features=" << f->values.size() << "\n";
    }

    return 0;
}
