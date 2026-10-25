#pragma once
// ═══════════════════════════════════════════
// EXPLOSION — expanding ring + debris chunks
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <vector>
#include "Camera.hpp"

struct Debris {
    float x, y, vx, vy, rot, rotSpd, size;
};

class Explosion {
public:
    Explosion(float x, float y, float scale = 1.f);

    void update(float dt);
    void draw(sf::RenderTarget& rt, const Camera& cam);

    bool active = true;

private:
    float x_, y_, scale_;
    float t_ = 0.f;
    float dur_ = 0.6f;
    std::vector<Debris> debris_;
};
