#include "entities/Player.hpp"
#include "entities/EntityFactory.hpp"

entt::entity Player::spawn(entt::registry& registry, Vec2f pos) {
    m_entity = EntityFactory::createPlayer(registry, pos);
    return m_entity;
}

entt::entity Player::respawn(entt::registry& registry, Vec2f pos) {
    if (m_entity != entt::null && registry.valid(m_entity)) {
        registry.destroy(m_entity);
    }
    return spawn(registry, pos);
}

bool Player::isValid(const entt::registry& registry) const {
    return m_entity != entt::null && registry.valid(m_entity);
}
