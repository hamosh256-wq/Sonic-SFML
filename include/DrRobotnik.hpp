#pragma once
// ═══════════════════════════════════════════
// DR. ROBOTNIK — chunky pixel art, stationary defended NPC
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include "EventBus.hpp"
#include "Camera.hpp"

class DrRobotnik {
public:
    DrRobotnik(float x, float y);

    void takeDmg(float a);
    void update(float dt, EventBus& ev);
    void draw(sf::RenderTarget& rt, const Camera& cam);

    float x, y;
    float w = 44.f, h = 56.f;
    float hp = 150.f, maxHp = 150.f;
    float iframes = 0.f;
    bool active = true;

private:
    float rt_ = 0.f; // reinforcement timer
};
