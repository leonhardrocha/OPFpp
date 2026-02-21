#pragma once

#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cassert>

#include "IComponent.h"
#include "Entity.h"

namespace ecs {

// Base pool for type-erased component storage
struct IComponentPool {
    virtual ~IComponentPool() = default;
};

template<typename T>
struct ComponentPool : IComponentPool {
    std::unordered_map<EntityId, T> components;
};

class EntityRegistry {
public:
    EntityRegistry() = default;

    Entity create(EntityType type = EntityType::Unknown) {
        EntityId id = ++m_nextId;
        m_entities.emplace(id, type);
        return Entity{id, type};
    }

    bool destroy(EntityId id) {
        bool removed = m_entities.erase(id) > 0;
        if (removed) {
            // remove components from all pools
            for (auto &p : m_pools) {
                auto pool = p.second.get();
                // we can't type-erase removal easily here; try dynamic cast
                // but we can implement via template helper if needed.
            }
        }
        // user may remove components explicitly
        return removed;
    }

    EntityType getType(EntityId id) const {
        auto it = m_entities.find(id);
        if (it == m_entities.end()) return EntityType::Unknown;
        return it->second;
    }

    template<typename T, typename... Args>
    void addComponent(EntityId id, Args&&... args) {
        auto *pool = getOrCreatePool<T>();
        pool->components.emplace(id, T{std::forward<Args>(args)...});
    }

    template<typename T>
    bool hasComponent(EntityId id) const {
        auto *pool = getPoolIfExists<T>();
        if (!pool) return false;
        return pool->components.find(id) != pool->components.end();
    }

    template<typename T>
    T* getComponent(EntityId id) {
        auto *pool = getPoolIfExists<T>();
        if (!pool) return nullptr;
        auto it = pool->components.find(id);
        if (it == pool->components.end()) return nullptr;
        return &it->second;
    }

    template<typename T>
    const T* getComponent(EntityId id) const {
        auto *pool = getPoolIfExists<T>();
        if (!pool) return nullptr;
        auto it = pool->components.find(id);
        if (it == pool->components.end()) return nullptr;
        return &it->second;
    }

    template<typename T>
    void removeComponent(EntityId id) {
        auto *pool = getPoolIfExists<T>();
        if (!pool) return;
        pool->components.erase(id);
    }

    // Basic view: return all entity ids that have ALL requested components
    template<typename... Components>
    std::vector<EntityId> view() const {
        std::vector<EntityId> result;
        result.reserve(m_entities.size());
        for (auto &kv : m_entities) {
            EntityId id = kv.first;
            if (hasAll<Components...>(id)) result.push_back(id);
        }
        return result;
    }

private:
    template<typename T>
    ComponentPool<T>* getOrCreatePool() {
        std::type_index idx(typeid(T));
        auto it = m_pools.find(idx);
        if (it == m_pools.end()) {
            auto pool = std::make_unique<ComponentPool<T>>();
            ComponentPool<T>* ptr = pool.get();
            m_pools.emplace(idx, std::move(pool));
            return ptr;
        }
        return static_cast<ComponentPool<T>*>(it->second.get());
    }

    template<typename T>
    const ComponentPool<T>* getPoolIfExists() const {
        std::type_index idx(typeid(T));
        auto it = m_pools.find(idx);
        if (it == m_pools.end()) return nullptr;
        return static_cast<const ComponentPool<T>*>(it->second.get());
    }

    template<typename T>
    ComponentPool<T>* getPoolIfExists() {
        std::type_index idx(typeid(T));
        auto it = m_pools.find(idx);
        if (it == m_pools.end()) return nullptr;
        return static_cast<ComponentPool<T>*>(it->second.get());
    }

    template<typename T>
    bool hasAll(EntityId id) const {
        return hasComponent<T>(id);
    }

    template<typename First, typename Second, typename... Rest>
    bool hasAll(EntityId id) const {
        return hasComponent<First>(id) && hasAll<Second, Rest...>(id);
    }

private:
    EntityId m_nextId{0};
    std::unordered_map<EntityId, EntityType> m_entities;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
};

} // namespace ecs
