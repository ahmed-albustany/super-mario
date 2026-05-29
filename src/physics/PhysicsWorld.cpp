#include "physics/PhysicsWorld.hpp"

void PhysicsWorld::reset() {
    gravity          = Config::GRAVITY;
    terminalVelocity = Config::TERMINAL_VELOCITY;
    wallSlideGravity = Config::WALL_SLIDE_GRAVITY;
    groundFriction   = 0.85f;
    airFriction      = 0.98f;
    iceMultiplier    = 1.0f;
}

void PhysicsWorld::loadFromLevel(float levelGravity) {
    if (levelGravity > 0.0f) {
        gravity = levelGravity;
    }
}
