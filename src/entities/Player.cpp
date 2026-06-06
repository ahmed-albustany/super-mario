#include "entities/Player.hpp"
#include "entities/EntityFactory.hpp"

entt::entity Player::spawn(entt::registry& registry, Vec2f pos, int playerIndex) {
    m_entity = EntityFactory::createPlayer(registry, pos, playerIndex);
    return m_entity;
}

entt::entity Player::respawn(entt::registry& registry, Vec2f pos, int playerIndex) {
    if (m_entity != entt::null && registry.valid(m_entity)) {
        registry.destroy(m_entity);
    }
    return spawn(registry, pos, playerIndex);
}

bool Player::isValid(const entt::registry& registry) const {
    return m_entity != entt::null && registry.valid(m_entity);
}
