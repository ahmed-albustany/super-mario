#pragma once

#include <string>
#include <optional>
#include <fstream>
#include <sstream>
#include <iostream>
#include "utils/Logger.hpp"

#ifndef MARIO_WASM
#include <filesystem>
#endif

/// @brief Safe file I/O — all paths validated against a whitelist root.
///        No ../ traversal, no absolute paths from user input.
namespace SafeIO {

namespace detail {
    inline std::string& rootPath() {
        static std::string root;
        return root;
    }
} // namespace detail

/// @brief Set the root assets directory (absolute path). Called once at startup.
inline void setRoot(const std::string& absPath) {
#ifdef MARIO_WASM
    // In WASM, std::filesystem is unreliable. Use the path as-is.
    detail::rootPath() = absPath;
    // Ensure trailing slash
    if (!detail::rootPath().empty() && detail::rootPath().back() != '/') {
        detail::rootPath() += '/';
    }
#else
    namespace fs = std::filesystem;
    try {
        detail::rootPath() = fs::canonical(fs::path(absPath)).string();
    } catch (const fs::filesystem_error&) {
        // If canonical fails (path doesn't exist yet), use the absolute path
        detail::rootPath() = fs::absolute(fs::path(absPath)).string();
    }
#endif
    std::cerr << "SafeIO root set to: " << detail::rootPath() << std::endl;
}

/// @brief Validate and resolve a relative path within the root.
///        Returns std::nullopt if the path escapes root, contains .., or is absolute.
[[nodiscard]] inline std::optional<std::string> safePath(const std::string& relativePath) {
    const auto& root = detail::rootPath();

    if (root.empty()) {
        LOG_ERROR("SafeIO: root not set");
        return std::nullopt;
    }

    // Reject explicit traversal
    if (relativePath.find("..") != std::string::npos) {
        LOG_ERROR("SafeIO: rejected path with '..'");
        return std::nullopt;
    }

#ifdef MARIO_WASM
    // In WASM, reject absolute paths manually
    if (!relativePath.empty() && relativePath[0] == '/') {
        LOG_ERROR("SafeIO: rejected absolute path");
        return std::nullopt;
    }
    // Simple concatenation — root already has trailing slash
    return root + relativePath;
#else
    namespace fs = std::filesystem;

    // Reject absolute paths
    if (fs::path(relativePath).is_absolute()) {
        LOG_ERROR("SafeIO: rejected absolute path");
        return std::nullopt;
    }

    // Build the full path and canonicalize
    fs::path full = fs::path(root) / relativePath;

    try {
        // If file exists, use canonical (resolves symlinks)
        if (fs::exists(full)) {
            std::string canonical = fs::canonical(full).string();
            // Ensure it starts with root
            if (canonical.rfind(root, 0) != 0) {
                LOG_ERROR("SafeIO: path escapes root after canonicalization");
                return std::nullopt;
            }
            return canonical;
        }

        // File doesn't exist yet (for writes) — use weakly_canonical
        std::string weakCanon = fs::weakly_canonical(full).string();
        // Normalize separators for comparison
        std::string normalizedRoot = root;
        std::replace(normalizedRoot.begin(), normalizedRoot.end(), '\\', '/');
        std::replace(weakCanon.begin(), weakCanon.end(), '\\', '/');

        if (weakCanon.rfind(normalizedRoot, 0) != 0) {
            LOG_ERROR("SafeIO: path escapes root");
            return std::nullopt;
        }
        return fs::weakly_canonical(full).string();

    } catch (const fs::filesystem_error&) {
        LOG_ERROR("SafeIO: filesystem error during path resolution");
        return std::nullopt;
    }
#endif
}

/// @brief Read entire file as string. Returns std::nullopt on any error.
[[nodiscard]] inline std::optional<std::string> readFile(const std::string& relativePath) {
    auto resolved = safePath(relativePath);
    if (!resolved) return std::nullopt;

    std::ifstream file(*resolved, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("SafeIO: failed to open file for reading");
        return std::nullopt;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

/// @brief Write string to file. Returns false on error.
[[nodiscard]] inline bool writeFile(const std::string& relativePath, const std::string& data) {
    auto resolved = safePath(relativePath);
    if (!resolved) return false;

#ifndef MARIO_WASM
    namespace fs = std::filesystem;

    // Create parent directories if needed
    fs::path parent = fs::path(*resolved).parent_path();
    if (!parent.empty() && !fs::exists(parent)) {
        try {
            fs::create_directories(parent);
        } catch (const fs::filesystem_error&) {
            LOG_ERROR("SafeIO: failed to create directories for write");
            return false;
        }
    }
#endif

    std::ofstream file(*resolved, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        LOG_ERROR("SafeIO: failed to open file for writing");
        return false;
    }

    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    return file.good();
}

} // namespace SafeIO
