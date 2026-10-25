#pragma once
// ═══════════════════════════════════════════
// IMPACT STAR — hit confirmation burst
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <vector>
#include "Camera.hpp"

struct StarLine {
    float a;
    float len;
};

class ImpactStar {
public:
    ImpactStar(float x, float y, sf::Color col = sf::Color::White, bool big = false);

    void update(float dt);
    void draw(sf::RenderTarget& rt, const Camera& cam);

    bool active = true;

private:
    float x_, y_;
    sf::Color col_;
    float life_, ml_;
    std::vector<StarLine> lines_;
};
