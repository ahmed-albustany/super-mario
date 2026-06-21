#include <gtest/gtest.h>
#include "world/LevelLoader.hpp"
#include "utils/SafeFileIO.hpp"

#include <filesystem>

// =============================================================================
// Fixture — sets up SafeIO root so LevelLoader can find files
// =============================================================================

class LevelLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set SafeIO root to the project's assets directory.
        // Works from the build directory (assets are copied there by CMake).
        namespace fs = std::filesystem;

        // Try multiple candidate paths for the assets directory
        std::vector<std::string> candidates = {
            "assets",
            "../assets",
            "../../assets",
        };

        for (const auto& path : candidates) {
            if (fs::exists(fs::path(path) / "levels" / "level_01.json")) {
                SafeIO::setRoot(path);
                m_rootSet = true;
                return;
            }
        }

        // Fallback: set to "assets" even if not found — tests that need
        // the file will fail gracefully; tests that don't need it will pass.
        SafeIO::setRoot("assets");
    }

    bool m_rootSet = false;
};

// =============================================================================
// Valid level loading
// =============================================================================

TEST_F(LevelLoaderTest, ValidLevelLoadsSuccessfully) {
    if (!m_rootSet) GTEST_SKIP() << "assets directory not found";

    auto result = LevelLoader::load("levels/level_01.json");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->name.empty());
}

TEST_F(LevelLoaderTest, PlayerSpawnNonZero) {
    if (!m_rootSet) GTEST_SKIP() << "assets directory not found";

    auto result = LevelLoader::load("levels/level_01.json");
    ASSERT_TRUE(result.has_value());

    // Player spawn should have been converted to pixel coords (non-zero)
    EXPECT_GT(result->playerSpawn.x, 0.0f);
    EXPECT_GT(result->playerSpawn.y, 0.0f);
}

TEST_F(LevelLoaderTest, FruitListNonEmpty) {
    if (!m_rootSet) GTEST_SKIP() << "assets directory not found";

    auto result = LevelLoader::load("levels/level_01.json");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->fruits.empty());
}

TEST_F(LevelLoaderTest, TrapListNonEmpty) {
    if (!m_rootSet) GTEST_SKIP() << "assets directory not found";

    auto result = LevelLoader::load("levels/level_01.json");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->traps.empty());
}

TEST_F(LevelLoaderTest, TilesAreLoaded) {
    if (!m_rootSet) GTEST_SKIP() << "assets directory not found";

    auto result = LevelLoader::load("levels/level_01.json");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->tiles.empty());
}

TEST_F(LevelLoaderTest, LevelDimensionsReasonable) {
    if (!m_rootSet) GTEST_SKIP() << "assets directory not found";

    auto result = LevelLoader::load("levels/level_01.json");
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->widthTiles, 0);
    EXPECT_GT(result->heightTiles, 0);
    EXPECT_LE(result->widthTiles, 10000);
    EXPECT_LE(result->heightTiles, 1000);
}

// =============================================================================
// Error handling — no crashes
// =============================================================================

TEST_F(LevelLoaderTest, NonexistentFileReturnsNullopt) {
    auto result = LevelLoader::load("levels/does_not_exist.json");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LevelLoaderTest, EmptyPathReturnsNullopt) {
    auto result = LevelLoader::load("");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LevelLoaderTest, PathTraversalBlocked) {
    auto result = LevelLoader::load("../../etc/passwd");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LevelLoaderTest, DoubleDotsInMiddleBlocked) {
    auto result = LevelLoader::load("levels/../../../etc/shadow");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LevelLoaderTest, MalformedJsonReturnsNullopt) {
    // Write a temporary malformed JSON file via SafeIO
    // This test verifies the JSON parser doesn't crash
    bool wrote = SafeIO::writeFile("levels/_test_malformed.json", "{{{not valid json!!");
    if (!wrote) GTEST_SKIP() << "Could not write temp test file";

    auto result = LevelLoader::load("levels/_test_malformed.json");
    EXPECT_FALSE(result.has_value());

    // Cleanup
    auto resolved = SafeIO::safePath("levels/_test_malformed.json");
    if (resolved) {
        std::filesystem::remove(*resolved);
    }
}

TEST_F(LevelLoaderTest, MissingMetaSectionReturnsNullopt) {
    // Valid JSON but missing required 'meta' section
    std::string json = R"({
        "tiles": [],
        "spawn_points": {}
    })";

    bool wrote = SafeIO::writeFile("levels/_test_no_meta.json", json);
    if (!wrote) GTEST_SKIP() << "Could not write temp test file";

    auto result = LevelLoader::load("levels/_test_no_meta.json");
    EXPECT_FALSE(result.has_value());

    auto resolved = SafeIO::safePath("levels/_test_no_meta.json");
    if (resolved) {
        std::filesystem::remove(*resolved);
    }
}

TEST_F(LevelLoaderTest, MissingTilesSectionReturnsNullopt) {
    std::string json = R"({
        "meta": {"name": "test", "width": 10, "height": 10},
        "spawn_points": {}
    })";

    bool wrote = SafeIO::writeFile("levels/_test_no_tiles.json", json);
    if (!wrote) GTEST_SKIP() << "Could not write temp test file";

    auto result = LevelLoader::load("levels/_test_no_tiles.json");
    EXPECT_FALSE(result.has_value());

    auto resolved = SafeIO::safePath("levels/_test_no_tiles.json");
    if (resolved) {
        std::filesystem::remove(*resolved);
    }
}

TEST_F(LevelLoaderTest, MissingSpawnPointsSectionReturnsNullopt) {
    std::string json = R"({
        "meta": {"name": "test", "width": 10, "height": 10},
        "tiles": []
    })";

    bool wrote = SafeIO::writeFile("levels/_test_no_spawns.json", json);
    if (!wrote) GTEST_SKIP() << "Could not write temp test file";

    auto result = LevelLoader::load("levels/_test_no_spawns.json");
    EXPECT_FALSE(result.has_value());

    auto resolved = SafeIO::safePath("levels/_test_no_spawns.json");
    if (resolved) {
        std::filesystem::remove(*resolved);
    }
}

TEST_F(LevelLoaderTest, EmptyButValidJsonReturnsNullopt) {
    bool wrote = SafeIO::writeFile("levels/_test_empty.json", "{}");
    if (!wrote) GTEST_SKIP() << "Could not write temp test file";

    auto result = LevelLoader::load("levels/_test_empty.json");
    EXPECT_FALSE(result.has_value());

    auto resolved = SafeIO::safePath("levels/_test_empty.json");
    if (resolved) {
        std::filesystem::remove(*resolved);
    }
}
