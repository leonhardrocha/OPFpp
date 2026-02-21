#pragma once
#include <cstdint>

namespace ecs {

using EntityId = uint32_t;

enum class EntityType : uint8_t {
    Unknown = 0,
    Sample,
    Dataset,
    Model,
    Subgraph,
    DistanceJob,
    Experiment
};

struct Entity {
    EntityId id{0};
    EntityType type{EntityType::Unknown};

    Entity() = default;
    Entity(EntityId i, EntityType t) : id(i), type(t) {}
};

} // namespace ecs
