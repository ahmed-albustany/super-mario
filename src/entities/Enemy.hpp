#pragma once

#include "ecs/Components.hpp"
#include "utils/Math.hpp"
#include <entt/entt.hpp>

/// @brief Lightweight wrapper around an enemy entity.
///        Holds the entity handle and enemy type for quick identification.
///        Used internally by EntityFactory — no game logic here.
class Enemy {
public:
    Enemy() = default;
    Enemy(entt::entity entity, EnemyType type)
        : m_entity(entity), m_type(type) {}

    /// @brief Get the underlying entity handle.
    [[nodiscard]] entt::entity getEntity() const { return m_entity; }

    /// @brief Get the enemy type.
    [[nodiscard]] EnemyType getType() const { return m_type; }

    /// @brief Check if the entity handle is valid in the given registry.
    [[nodiscard]] bool isValid(const entt::registry& registry) const {
        return m_entity != entt::null && registry.valid(m_entity);
    }

private:
    entt::entity m_entity = entt::null;
    EnemyType    m_type   = EnemyType::Goomba;
};
