#pragma once

#include <cstdint>
#include <string>

/// @brief All game constants live here. No magic numbers elsewhere in the codebase.
namespace Config {

    // ---- Window ----
    constexpr int   WINDOW_WIDTH         = 1280;
    constexpr int   WINDOW_HEIGHT        = 720;
    constexpr int   TARGET_FPS           = 60;
    constexpr float FIXED_TIMESTEP       = 1.0f / 60.0f;
    constexpr float MAX_DELTA_TIME       = 1.0f / 30.0f;  // spiral-of-death guard

    // ---- Physics (Mario-style) ----
    constexpr float GRAVITY              = 2400.0f;   // px/s^2 — snappier than before
    constexpr float TERMINAL_VELOCITY    = 900.0f;    // px/s
    constexpr float PLAYER_WALK_SPEED    = 200.0f;    // px/s walking
    constexpr float PLAYER_RUN_SPEED     = 380.0f;    // px/s running (hold Run button)
    constexpr float PLAYER_JUMP_FORCE    = -680.0f;   // px/s (negative = up)
    constexpr float PLAYER_JUMP_CUT      = 0.4f;      // velocity multiplier on early release
    constexpr float PLAYER_STOMP_BOUNCE  = -400.0f;   // bounce velocity after stomping enemy
    constexpr float PLAYER_ACCEL         = 1200.0f;   // px/s^2 ground acceleration
    constexpr float PLAYER_DECEL         = 1800.0f;   // px/s^2 skid deceleration
    constexpr float PLAYER_AIR_ACCEL     = 800.0f;    // px/s^2 air acceleration
    constexpr int   MAX_JUMPS            = 1;         // single jump only

    // ---- Fireball ----
    constexpr float FIREBALL_SPEED       = 400.0f;    // px/s
    constexpr float FIREBALL_BOUNCE_VY   = -300.0f;   // bounce off ground
    constexpr float FIREBALL_GRAVITY     = 1800.0f;
    constexpr float FIREBALL_LIFETIME    = 3.0f;

    // ---- Star (invincibility) ----
    constexpr float STAR_DURATION        = 10.0f;     // seconds

    // ---- Question Block ----
    constexpr float BLOCK_BUMP_SPEED     = -200.0f;   // bump animation speed
    constexpr float BLOCK_BUMP_DURATION  = 0.2f;

    // ---- Tile ----
    constexpr int   TILE_SIZE            = 32;        // pixels
    constexpr int   LEVEL_WIDTH_TILES    = 200;
    constexpr int   LEVEL_HEIGHT_TILES   = 15;

    // ---- Camera ----
    constexpr float CAMERA_LERP          = 6.0f;      // lerp speed multiplier
    constexpr float SCREEN_SHAKE_DECAY   = 8.0f;      // decay rate

    // ---- Audio ----
    constexpr int   MAX_SOUND_CHANNELS   = 16;
    constexpr float DEFAULT_MUSIC_VOL    = 0.75f;     // 0.0 – 1.0
    constexpr float DEFAULT_SFX_VOL      = 1.0f;      // 0.0 – 1.0

    // ---- Gameplay ----
    constexpr int   INVINCIBILITY_FRAMES     = 120;    // frames of i-frames after hit
    constexpr int   COIN_VALUE               = 200;    // score points
    constexpr int   COIN_EXTRA_LIFE          = 100;    // coins needed for 1-up
    constexpr int   ENEMY_STOMP_VALUE        = 100;
    constexpr int   BLOCK_HIT_VALUE          = 10;
    constexpr int   FIREBALL_KILL_VALUE      = 200;
    constexpr int   FLAGPOLE_BASE_SCORE      = 2000;
    constexpr int   DEFAULT_LIVES            = 3;
    constexpr float DEFAULT_TIME_LIMIT       = 300.0f; // seconds per level

    // ---- Save ----
    constexpr uint32_t SAVE_MAGIC    = 0xDEADC0DE;
    constexpr int      SAVE_VERSION  = 2;

    // ---- Identity ----
    inline const std::string GAME_TITLE   = "Super Mario Bros";
    inline const std::string GAME_VERSION = "2.0.0";

} // namespace Config
