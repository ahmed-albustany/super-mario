#pragma once

#include <string>
#include <array>
#include <fstream>
#include <sstream>

#ifdef MARIO_WASM
#include <emscripten.h>
#include <cstdlib>
#else
#include <nlohmann/json.hpp>
#endif

#include "utils/Logger.hpp"

/// @brief Persistent save data for the game.
struct SaveData {
    int highestLevel = 0;
    int highScore    = 0;
    std::array<bool, 10> levelCompleted = {};
};

/// @brief Singleton save manager.
///        Uses localStorage on WASM builds, a JSON file on native builds.
class SaveManager {
public:
    /// @brief Get the singleton instance.
    static SaveManager& instance() {
        static SaveManager sm;
        return sm;
    }

    // Non-copyable
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;

    /// @brief Persist the given save data.
    void save(const SaveData& data) {
        std::string json = serialise(data);

#ifdef MARIO_WASM
        EM_ASM({
            localStorage.setItem('pixel_rush_save', UTF8ToString($0));
        }, json.c_str());
        LOG_INFO("SaveManager: saved to localStorage");
#else
        std::ofstream out(nativePath());
        if (!out) {
            LOG_ERROR("SaveManager: failed to open " << nativePath() << " for writing");
            return;
        }
        out << json;
        LOG_INFO("SaveManager: saved to " << nativePath());
#endif
    }

    /// @brief Load save data from storage. Returns defaults if nothing is stored.
    SaveData load() {
#ifdef MARIO_WASM
        char* raw = (char*)EM_ASM_PTR({
            var s = localStorage.getItem('pixel_rush_save');
            if (!s) return 0;
            var len = lengthBytesUTF8(s) + 1;
            var buf = _malloc(len);
            stringToUTF8(s, buf, len);
            return buf;
        });

        if (!raw) {
            LOG_INFO("SaveManager: no save found in localStorage, using defaults");
            return {};
        }

        std::string json(raw);
        free(raw);
        LOG_INFO("SaveManager: loaded from localStorage");
        return deserialise(json);
#else
        std::ifstream in(nativePath());
        if (!in) {
            LOG_INFO("SaveManager: no save file found, using defaults");
            return {};
        }

        std::ostringstream buf;
        buf << in.rdbuf();
        LOG_INFO("SaveManager: loaded from " << nativePath());
        return deserialise(buf.str());
#endif
    }

private:
    SaveManager() = default;

    // ---- Serialisation helpers (hand-rolled for WASM, nlohmann for native) ----

    static std::string serialise(const SaveData& d) {
#ifdef MARIO_WASM
        // Minimal hand-rolled JSON so we don't need nlohmann on WASM.
        std::ostringstream os;
        os << "{\"highestLevel\":" << d.highestLevel
           << ",\"highScore\":"   << d.highScore
           << ",\"levelCompleted\":[";
        for (std::size_t i = 0; i < d.levelCompleted.size(); ++i) {
            if (i > 0) os << ',';
            os << (d.levelCompleted[i] ? "true" : "false");
        }
        os << "]}";
        return os.str();
#else
        nlohmann::json j;
        j["highestLevel"]   = d.highestLevel;
        j["highScore"]      = d.highScore;
        j["levelCompleted"] = d.levelCompleted;
        return j.dump();
#endif
    }

    static SaveData deserialise(const std::string& json) {
        SaveData d;
        try {
#ifdef MARIO_WASM
            // Tiny hand-rolled parser — fields are always written by serialise().
            auto readInt = [&](const std::string& key) -> int {
                auto pos = json.find("\"" + key + "\":");
                if (pos == std::string::npos) return 0;
                pos = json.find(':', pos) + 1;
                return std::stoi(json.substr(pos));
            };
            d.highestLevel = readInt("highestLevel");
            d.highScore    = readInt("highScore");

            auto arrStart = json.find("\"levelCompleted\":[");
            if (arrStart != std::string::npos) {
                arrStart = json.find('[', arrStart) + 1;
                for (std::size_t i = 0; i < d.levelCompleted.size(); ++i) {
                    while (arrStart < json.size() && (json[arrStart] == ' ' || json[arrStart] == ','))
                        ++arrStart;
                    d.levelCompleted[i] = (json.compare(arrStart, 4, "true") == 0);
                    arrStart += d.levelCompleted[i] ? 4 : 5; // skip "true" or "false"
                }
            }
#else
            auto j = nlohmann::json::parse(json);
            d.highestLevel   = j.value("highestLevel", 0);
            d.highScore      = j.value("highScore", 0);
            if (j.contains("levelCompleted")) {
                auto& arr = j["levelCompleted"];
                for (std::size_t i = 0; i < d.levelCompleted.size() && i < arr.size(); ++i) {
                    d.levelCompleted[i] = arr[i].get<bool>();
                }
            }
#endif
        } catch (const std::exception& e) {
            LOG_ERROR("SaveManager: failed to parse save data: " << e.what());
        }
        return d;
    }

#ifndef MARIO_WASM
    /// @brief Path to the native save file (next to the executable).
    static std::string nativePath() {
        static const std::string path = "save.json";
        return path;
    }
#endif
};
