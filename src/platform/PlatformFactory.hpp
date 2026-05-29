#pragma once

#include <memory>
#include "platform/IPlatform.hpp"

/// @brief Compile-time platform selection.
///        Returns the correct IPlatform implementation based on MARIO_WASM.
namespace Platform {

inline std::unique_ptr<IPlatform> create() {
#ifdef MARIO_WASM
    // Forward declaration — include kept out of header to avoid
    // leaking SDL headers into game code.
    std::unique_ptr<IPlatform> createSDLPlatform();
    return createSDLPlatform();
#else
    // Forward declaration — include kept out of header to avoid
    // leaking SFML headers into game code.
    std::unique_ptr<IPlatform> createSFMLPlatform();
    return createSFMLPlatform();
#endif
}

} // namespace Platform
