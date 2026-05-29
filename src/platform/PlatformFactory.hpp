#pragma once

#include <memory>
#include "platform/IPlatform.hpp"

// Forward declarations in global namespace — definitions live in
// SFMLPlatform.cpp / SDLPlatform.cpp. Kept out of header to avoid
// leaking SFML/SDL headers into game code.
#ifdef MARIO_WASM
std::unique_ptr<IPlatform> createSDLPlatform();
#else
std::unique_ptr<IPlatform> createSFMLPlatform();
#endif

/// @brief Compile-time platform selection.
///        Returns the correct IPlatform implementation based on MARIO_WASM.
namespace Platform {

inline std::unique_ptr<IPlatform> create() {
#ifdef MARIO_WASM
    return createSDLPlatform();
#else
    return createSFMLPlatform();
#endif
}

} // namespace Platform
