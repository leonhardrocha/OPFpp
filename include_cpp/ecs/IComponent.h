#pragma once

namespace ecs {

struct IComponent {
    virtual ~IComponent() = default;
};

} // namespace ecs
