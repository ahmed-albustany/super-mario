#include "core/Game.hpp"
#include "core/GameConfig.hpp"
#include "core/InputManager.hpp"
#include "platform/PlatformFactory.hpp"
#include "utils/Logger.hpp"

#ifdef MARIO_WASM
#include <emscripten.h>

static Game* g_game = nullptr;

static void emscriptenLoop() {
    if (g_game && g_game->isRunning()) {
        g_game->tick();
    }
}

// Export touch input bridge for the web touch overlay JS.
// Called via Module.ccall('setTouchInput', null, ['number','number'], [actionId, pressed]);
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void setTouchInput(int action, int pressed) {
        if (g_game && action >= 0 && action < static_cast<int>(Action::COUNT)) {
            g_game->input().setTouchButtonState(
                static_cast<Action>(action), pressed != 0);
        }
    }
}
#endif

int main() {
    LOG_INFO("Starting " << Config::GAME_TITLE << " v" << Config::GAME_VERSION);

    auto platform = Platform::create();
    Game game(std::move(platform));

#ifdef MARIO_WASM
    g_game = &game;
    emscripten_set_main_loop(emscriptenLoop, 0, 1);
#else
    while (game.isRunning()) {
        game.tick();
    }
#endif

    LOG_INFO("Shutdown complete.");
    return 0;
}
