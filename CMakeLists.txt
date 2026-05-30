cmake_minimum_required(VERSION 3.20)
project(MarioGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Options
# =============================================================================
option(MARIO_BUILD_TESTS "Build unit tests" ON)
option(MARIO_BUILD_WASM "Build for WebAssembly via Emscripten" OFF)
option(MARIO_ENABLE_SANITIZERS "Enable ASan + UBSan in debug builds" ON)

# =============================================================================
# Compiler warnings — zero tolerance
# =============================================================================
if(MSVC)
    set(MARIO_WARNING_FLAGS /W4 /WX)
else()
    set(MARIO_WARNING_FLAGS
        -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor
        -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual
        -Wconversion -Wsign-conversion -Wnull-dereference
        -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough
    )
endif()

# =============================================================================
# Sanitizers (debug, native only, not MSVC)
# =============================================================================
set(MARIO_SANITIZER_FLAGS "")
if(MARIO_ENABLE_SANITIZERS AND NOT MARIO_BUILD_WASM AND NOT MSVC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            set(MARIO_SANITIZER_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
        endif()
    endif()
endif()

# =============================================================================
# Dependencies (fetched automatically)
# =============================================================================
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# EnTT
FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.2
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(nlohmann_json EnTT)

# =============================================================================
# Source files — Step 2 (core engine + platform layer)
# Platform sources are conditionally selected below.
# =============================================================================
set(MARIO_CORE_SOURCES
    src/core/Game.cpp
    src/core/ResourceManager.cpp
    src/core/InputManager.cpp
    src/core/SceneManager.cpp
)

set(MARIO_ECS_SOURCES
    src/ecs/systems/PhysicsSystem.cpp
    src/ecs/systems/CollisionSystem.cpp
    src/ecs/systems/MovementSystem.cpp
    src/ecs/systems/PlayerSystem.cpp
    src/ecs/systems/AnimationSystem.cpp
    src/ecs/systems/EnemyAISystem.cpp
    src/ecs/systems/RenderSystem.cpp
    src/ecs/systems/PowerUpSystem.cpp
)

set(MARIO_WORLD_SOURCES
    src/physics/PhysicsWorld.cpp
    src/world/TileMap.cpp
    src/world/LevelLoader.cpp
    src/world/Camera.cpp
    src/world/Parallax.cpp
)

set(MARIO_SCENE_SOURCES
    src/scenes/BootScene.cpp
    src/scenes/MenuScene.cpp
    src/scenes/GameScene.cpp
    src/scenes/PauseScene.cpp
    src/scenes/HUDScene.cpp
    src/scenes/GameOverScene.cpp
)

set(MARIO_ENTITY_SOURCES
    src/entities/EntityFactory.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
)

set(MARIO_AUDIO_SOURCES
    src/audio/AudioManager.cpp
)

set(MARIO_UI_SOURCES
    src/ui/Button.cpp
    src/ui/Panel.cpp
    src/ui/Text.cpp
)

set(MARIO_LIB_SOURCES
    ${MARIO_CORE_SOURCES}
    ${MARIO_ECS_SOURCES}
    ${MARIO_WORLD_SOURCES}
    ${MARIO_SCENE_SOURCES}
    ${MARIO_ENTITY_SOURCES}
    ${MARIO_AUDIO_SOURCES}
    ${MARIO_UI_SOURCES}
)

# =============================================================================
# WASM (Emscripten) build
# =============================================================================
if(MARIO_BUILD_WASM)
    # SDL Platform
    set(MARIO_PLATFORM_SOURCES src/platform/SDLPlatform.cpp)

    add_library(MarioGameLib STATIC
        ${MARIO_LIB_SOURCES}
        ${MARIO_PLATFORM_SOURCES}
    )

    target_include_directories(MarioGameLib PUBLIC ${CMAKE_SOURCE_DIR}/src)

    target_link_libraries(MarioGameLib PUBLIC
        nlohmann_json::nlohmann_json
        EnTT::EnTT
    )

    target_compile_definitions(MarioGameLib PUBLIC MARIO_WASM=1)
    target_compile_options(MarioGameLib PRIVATE ${MARIO_WARNING_FLAGS})

    # Main executable
    add_executable(MarioGame-web src/main.cpp)
    target_link_libraries(MarioGame-web PRIVATE MarioGameLib)
    target_compile_options(MarioGame-web PRIVATE ${MARIO_WARNING_FLAGS})
    target_compile_definitions(MarioGame-web PRIVATE MARIO_WASM=1)

    # Emscripten link flags (use -sFLAG=VALUE format — no space after -s)
    set(MARIO_EM_FLAGS
        "-sUSE_SDL=2"
        "-sUSE_SDL_IMAGE=2"
        "-sSDL2_IMAGE_FORMATS=[\"png\"]"
        "-sUSE_SDL_TTF=2"
        "-sUSE_SDL_MIXER=2"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=67108864"
        "-sEXPORTED_FUNCTIONS=[\"_main\",\"_setTouchInput\"]"
        "-sEXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\"]"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets@/assets"
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND MARIO_EM_FLAGS "-sASSERTIONS=1" "-g2")
    else()
        list(APPEND MARIO_EM_FLAGS "-O2" "--closure 1")
    endif()

    target_link_options(MarioGame-web PRIVATE ${MARIO_EM_FLAGS})

    # SDL2 port header availability for compilation (single flag, no space)
    target_compile_options(MarioGameLib PRIVATE
        "-sUSE_SDL=2"
        "-sUSE_SDL_IMAGE=2"
        "-sUSE_SDL_TTF=2"
        "-sUSE_SDL_MIXER=2"
    )

    # Suppress third-party (EnTT) sign-conversion warnings in WASM builds
    target_compile_options(MarioGameLib PRIVATE
        -Wno-sign-conversion -Wno-conversion -Wno-implicit-int-float-conversion
    )

    set_target_properties(MarioGame-web PROPERTIES
        OUTPUT_NAME "mario"
        SUFFIX ".html"
    )

    # We use our own web shell — copy index.html, style.css, loading.js to build dir
    add_custom_command(TARGET MarioGame-web POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/web/index.html $<TARGET_FILE_DIR:MarioGame-web>/index.html
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/web/style.css  $<TARGET_FILE_DIR:MarioGame-web>/style.css
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/web/loading.js $<TARGET_FILE_DIR:MarioGame-web>/loading.js
        COMMENT "Copying web shell files to WASM build directory"
    )

# =============================================================================
# Native build (SFML)
# =============================================================================
else()
    # SFML Platform
    set(MARIO_PLATFORM_SOURCES src/platform/SFMLPlatform.cpp)

    # Find SFML — try system first, then FetchContent fallback
    find_package(SFML 2.6 COMPONENTS graphics window audio system QUIET)

    if(NOT SFML_FOUND)
        message(STATUS "SFML not found via find_package, fetching via FetchContent...")
        FetchContent_Declare(
            SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG        2.6.1
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(SFML)
    endif()

    # ---- Static library (all game code, used by tests + exe) ----
    add_library(MarioGameLib STATIC
        ${MARIO_LIB_SOURCES}
        ${MARIO_PLATFORM_SOURCES}
    )

    target_include_directories(MarioGameLib PUBLIC ${CMAKE_SOURCE_DIR}/src)

    target_link_libraries(MarioGameLib PUBLIC
        sfml-graphics sfml-window sfml-audio sfml-system
        nlohmann_json::nlohmann_json
        EnTT::EnTT
    )

    target_compile_options(MarioGameLib PRIVATE
        ${MARIO_WARNING_FLAGS}
        ${MARIO_SANITIZER_FLAGS}
    )
    # Suppress conversion warnings from third-party headers (EnTT)
    if(NOT MSVC)
        target_compile_options(MarioGameLib PRIVATE
            -Wno-sign-conversion -Wno-conversion -Wno-implicit-int-float-conversion
        )
    endif()
    target_link_options(MarioGameLib PUBLIC ${MARIO_SANITIZER_FLAGS})

    # ---- Main executable ----
    add_executable(MarioGame src/main.cpp)
    target_link_libraries(MarioGame PRIVATE MarioGameLib)
    target_compile_options(MarioGame PRIVATE
        ${MARIO_WARNING_FLAGS}
        ${MARIO_SANITIZER_FLAGS}
    )

    # Copy assets to build output
    add_custom_command(TARGET MarioGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:MarioGame>/assets
        COMMENT "Copying assets to build directory"
    )

    # ---- Tests ----
    if(MARIO_BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

# =============================================================================
# Install
# =============================================================================
if(NOT MARIO_BUILD_WASM)
    install(TARGETS MarioGame RUNTIME DESTINATION bin)
    install(DIRECTORY assets/ DESTINATION bin/assets)
endif()
