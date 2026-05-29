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

    // ---- Physics ----
    constexpr float GRAVITY              = 1800.0f;   // px/s^2
    constexpr float TERMINAL_VELOCITY    = 900.0f;    // px/s
    constexpr float PLAYER_SPEED         = 260.0f;    // px/s
    constexpr float PLAYER_JUMP_FORCE    = -620.0f;   // px/s (negative = up)
    constexpr float PLAYER_DASH_SPEED    = 520.0f;    // px/s
    constexpr float PLAYER_DASH_DURATION = 0.22f;     // seconds
    constexpr float PLAYER_DASH_COOLDOWN = 0.9f;      // seconds
    constexpr float WALL_SLIDE_GRAVITY   = 200.0f;    // reduced gravity on wall
    constexpr float WALL_JUMP_FORCE_X    = 340.0f;    // px/s horizontal kick
    constexpr float WALL_JUMP_FORCE_Y    = -560.0f;   // px/s vertical kick
    constexpr int   MAX_JUMPS            = 2;         // double jump

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
    constexpr int   PLAYER_MAX_HP            = 3;
    constexpr int   INVINCIBILITY_FRAMES     = 90;     // frames of i-frames after hit
    constexpr float POWER_CRYSTAL_DURATION   = 10.0f;  // seconds
    constexpr int   COIN_VALUE               = 100;    // score points
    constexpr int   GEM_VALUE                = 500;
    constexpr int   ENEMY_STOMP_VALUE        = 200;
    constexpr int   DEFAULT_LIVES            = 3;
    constexpr float DEFAULT_TIME_LIMIT       = 300.0f; // seconds per level

    // ---- Save ----
    constexpr uint32_t SAVE_MAGIC    = 0xDEADC0DE;
    constexpr int      SAVE_VERSION  = 1;

    // ---- Identity ----
    inline const std::string GAME_TITLE   = "Ruins of the Ancients";
    inline const std::string GAME_VERSION = "1.0.0";

} // namespace Config
