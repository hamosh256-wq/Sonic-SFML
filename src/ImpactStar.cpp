#include "ImpactStar.hpp"
#include "Common.hpp"
#include <cmath>
#include <algorithm>

ImpactStar::ImpactStar(float x, float y, sf::Color col, bool big)
    : x_(x), y_(y), col_(col) {
    ml_ = big ? 0.32f : 0.2f;
    life_ = ml_;
    int n = big ? 8 : 6;
    float baseLen = big ? 26.f : 16.f;
    for (int i = 0; i < n; ++i) {
        float a = ((float)i / (float)n) * 3.14159265f * 2.f + gfx::frand(-0.15f, 0.15f);
        lines_.push_back(StarLine{a, baseLen * (0.7f + gfx::frand() * 0.6f)});
    }
}

void ImpactStar::update(float dt) {
    life_ -= dt;
    if (life_ <= 0.f) active = false;
}

void ImpactStar::draw(sf::RenderTarget& rt, const Camera& cam) {
    if (!active) return;
    float t = std::clamp(life_ / ml_, 0.f, 1.f);
    float cx = cam.wx(x_), cy = cam.wy(y_);
    sf::Color c = col_;
    c.a = (std::uint8_t)(t * 255.f);
    for (const auto& l : lines_) {
        sf::Vector2f dir{std::cos(l.a), std::sin(l.a)};
        float len = l.len * (1.f - t * 0.3f);
        gfx::thickLine(rt, {cx + dir.x * 3.f, cy + dir.y * 3.f},
                       {cx + dir.x * len, cy + dir.y * len}, 2.5f * t + 0.5f, c);
    }
    gfx::fillCircle(rt, {cx, cy}, 4.f * t, sf::Color(255, 255, 255, (std::uint8_t)(t * 220.f)));
}
