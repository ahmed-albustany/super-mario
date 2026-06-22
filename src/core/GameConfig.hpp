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

    // ---- Physics (Mario Bros style) ----
    constexpr float GRAVITY              = 1400.0f;   // px/s^2 (base, scales with fall multiplier)
    constexpr float TERMINAL_VELOCITY    = 700.0f;    // px/s max fall speed
    constexpr float PLAYER_WALK_SPEED    = 150.0f;    // px/s walking
    constexpr float PLAYER_RUN_SPEED     = 280.0f;    // px/s running (hold X / DoubleJump key)
    constexpr float PLAYER_JUMP_FORCE    = -520.0f;   // px/s (negative = up)
    constexpr float PLAYER_DOUBLE_JUMP_FORCE = -460.0f; // slightly weaker second jump
    constexpr float PLAYER_JUMP_CUT      = 0.45f;     // velocity multiplier on early release
    constexpr float PLAYER_RUN_JUMP_MULT = 1.20f;     // running jump force multiplier
    constexpr float PLAYER_STOMP_BOUNCE  = -400.0f;   // bounce velocity
    constexpr float PLAYER_ACCEL         = 1500.0f;   // px/s^2 ground acceleration (~6 frames 0→walk)
    constexpr float PLAYER_DECEL         = 2250.0f;   // px/s^2 ground deceleration (~4 frames walk→0)
    constexpr float PLAYER_AIR_ACCEL     = 900.0f;    // px/s^2 air acceleration (responsive but driftier)
    constexpr float PLAYER_AIR_DECEL     = 350.0f;    // px/s^2 air deceleration (slight air drift)
    constexpr float PLAYER_FALL_MULT     = 1.8f;      // gravity multiplier when falling
    constexpr float PLAYER_LOW_JUMP_MULT = 2.5f;      // gravity multiplier when jump released early
    constexpr int   MAX_JUMPS            = 2;         // double jump
    constexpr float TRAMPOLINE_FORCE     = -1560.0f;  // 3x normal jump height

    // ---- Coyote / Buffer (frames at 60fps) ----
    constexpr float COYOTE_TIME          = 0.1f;      // ~6 frames
    constexpr float JUMP_BUFFER          = 0.1f;      // ~6 frames

    // ---- Death animation ----
    constexpr float DEATH_HOP_FORCE      = -350.0f;
    constexpr float DEATH_RESPAWN_DELAY  = 2.0f;      // seconds

    // ---- Fruit scoring ----
    constexpr int   FRUIT_CHERRY_VALUE     = 100;
    constexpr int   FRUIT_APPLE_VALUE      = 200;
    constexpr int   FRUIT_ORANGE_VALUE     = 300;
    constexpr int   FRUIT_PINEAPPLE_VALUE  = 400;
    constexpr int   FRUIT_MELON_VALUE      = 500;
    constexpr int   FRUIT_STRAWBERRY_VALUE = 600;
    constexpr int   FRUIT_KIWI_VALUE       = 700;
    constexpr int   FRUIT_BANANA_VALUE     = 1000;
    constexpr int   EXTRA_LIFE_SCORE       = 10000;  // extra life every N points
    constexpr int   TROPHY_SCORE_MULTIPLIER = 2;     // first to reach trophy
    constexpr int   VS_DEATH_PENALTY       = 500;    // lose points on death in VS

    // ---- Box ----
    constexpr int   BOX_DEFAULT_HITS     = 3;
    constexpr float BLOCK_BUMP_SPEED     = -200.0f;
    constexpr float BLOCK_BUMP_DURATION  = 0.2f;

    // ---- Trap timers ----
    constexpr float FIRE_ON_TIME         = 2.0f;
    constexpr float FIRE_OFF_TIME        = 1.0f;
    constexpr float FALLING_PLATFORM_SHAKE_TIME = 1.0f;
    constexpr float SPIKE_HEAD_CHARGE_SPEED = 400.0f;

    // ---- Tile ----
    constexpr int   TILE_SIZE            = 16;        // Pixel Adventure uses 16px tiles
    constexpr int   LEVEL_WIDTH_TILES    = 60;
    constexpr int   LEVEL_HEIGHT_TILES   = 20;

    // ---- Camera ----
    constexpr float CAMERA_LERP          = 6.0f;
    constexpr float SCREEN_SHAKE_DECAY   = 8.0f;

    // ---- Audio ----
    constexpr int   MAX_SOUND_CHANNELS   = 16;
    constexpr float DEFAULT_MUSIC_VOL    = 0.75f;
    constexpr float DEFAULT_SFX_VOL      = 1.0f;

    // ---- Gameplay ----
    constexpr int   INVINCIBILITY_FRAMES     = 120;
    constexpr int   DEFAULT_LIVES            = 3;
    constexpr float DEFAULT_TIME_LIMIT       = 300.0f;
    constexpr int   MAX_PLAYERS              = 4;

    // ---- Legacy compatibility (unused but kept for compile) ----
    constexpr int   COIN_VALUE               = 200;
    constexpr int   COIN_EXTRA_LIFE          = 100;
    constexpr int   ENEMY_STOMP_VALUE        = 100;
    constexpr int   BLOCK_HIT_VALUE          = 10;
    constexpr int   FIREBALL_KILL_VALUE      = 200;
    constexpr int   FLAGPOLE_BASE_SCORE      = 2000;
    constexpr float FIREBALL_SPEED           = 400.0f;
    constexpr float FIREBALL_BOUNCE_VY       = -300.0f;
    constexpr float FIREBALL_GRAVITY         = 1800.0f;
    constexpr float FIREBALL_LIFETIME        = 3.0f;
    constexpr float STAR_DURATION            = 10.0f;

    // ---- Save ----
    constexpr uint32_t SAVE_MAGIC    = 0xDEADC0DE;
    constexpr int      SAVE_VERSION  = 3;

    // ---- Identity ----
    inline const std::string GAME_TITLE   = "PIXEL RUSH";
    inline const std::string GAME_VERSION = "1.0.0";

} // namespace Config
