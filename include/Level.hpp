#pragma once
// ═══════════════════════════════════════════
// LEVEL DATA
// ═══════════════════════════════════════════
#include <string>
#include <vector>
#include <optional>
#include "Physics.hpp"

struct RescueSpec {
    std::string name;
    float x, y;
};

struct DistortionZone {
    float x, y, r;
};

struct LevelData {
    std::string name;
    float w, h;
    std::string bg;
    float spawnX, spawnY;
    std::vector<Platform> platforms;
    std::optional<RescueSpec> rescue;
    std::vector<std::pair<float, float>> enemies;
    std::vector<DistortionZone> distortion;
};

const LevelData& getLevel(const std::string& key);
