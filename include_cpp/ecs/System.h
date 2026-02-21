#pragma once

#include "EntityRegistry.h"

namespace ecs {

struct ISystem {
    virtual ~ISystem() = default;
    virtual void update(EntityRegistry &registry, double /*dt*/) = 0;
};

} // namespace ecs
