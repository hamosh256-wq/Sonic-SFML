#pragma once
// ═══════════════════════════════════════════
// RESCUE TARGET — pulsing beacon NPC (Tails / Knuckles)
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <string>
#include "Camera.hpp"

class RescueTarget {
public:
    RescueTarget(float x, float y, std::string name);

    void update(float dt);
    void draw(sf::RenderTarget& rt, const Camera& cam, sf::Font& font);

    float x, y;
    std::string name;
    float w = 30.f, h = 42.f;
    bool active = true;
    bool rescued = false;
    float t = 0.f;
};
