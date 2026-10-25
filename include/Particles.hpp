#pragma once
// ═══════════════════════════════════════════
// PARTICLE POOL — bursts (radial) and sparks (directional)
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <vector>
#include "Camera.hpp"

struct Particle {
    float x, y, vx, vy;
    float life, maxLife;
    sf::Color col;
    float size;
    bool active = true;
};

class Particles {
public:
    void burst(float x, float y, int count, sf::Color col, float speed, float life);
    void spark(float x, float y, float dir, sf::Color col, int count = 3);
    void update(float dt);
    void draw(sf::RenderTarget& rt, const Camera& cam);

    std::vector<Particle> pool;
};
