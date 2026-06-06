#pragma once

#include "utils/Math.hpp"
#include <entt/entt.hpp>

/// @brief Thin wrapper around the player entity.
///        No game logic — that lives in PlayerSystem.
///        Provides spawn/respawn helpers and entity access.
class Player {
public:
    Player() = default;

    /// @brief Create the player entity in the registry at the given position.
    /// @param playerIndex 0 = Mario (P1), 1 = Luigi (P2)
    /// @return The newly created entity.
    entt::entity spawn(entt::registry& registry, Vec2f pos, int playerIndex = 0);

    /// @brief Destroy the current entity (if valid) and create a fresh one at pos.
    /// @return The newly created entity.
    entt::entity respawn(entt::registry& registry, Vec2f pos, int playerIndex = 0);

    /// @brief Get the underlying entity handle.
    [[nodiscard]] entt::entity getEntity() const { return m_entity; }

    /// @brief Check if the entity handle is valid in the given registry.
    [[nodiscard]] bool isValid(const entt::registry& registry) const;

private:
    entt::entity m_entity = entt::null;
};
