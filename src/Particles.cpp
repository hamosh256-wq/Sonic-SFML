#include "Particles.hpp"
#include "Common.hpp"
#include <cmath>
#include <algorithm>

void Particles::burst(float x, float y, int count, sf::Color col, float speed, float life) {
    for (int i = 0; i < count; ++i) {
        float a = gfx::frand() * 3.14159265f * 2.f;
        float s = speed * (0.4f + gfx::frand() * 0.6f);
        pool.push_back(Particle{
            x, y, std::cos(a) * s, std::sin(a) * s,
            life * (0.7f + gfx::frand() * 0.6f), life, col,
            2.f + gfx::frand() * 3.f, true
        });
    }
}

void Particles::spark(float x, float y, float dir, sf::Color col, int count) {
    for (int i = 0; i < count; ++i) {
        float a = 3.14159265f + dir * 1.5708f + (gfx::frand() - 0.5f) * 0.6f;
        float s = 60.f + gfx::frand() * 90.f;
        pool.push_back(Particle{
            x, y, std::cos(a) * s, std::sin(a) * s,
            0.25f + gfx::frand() * 0.2f, 0.4f, col,
            1.5f + gfx::frand() * 2.f, true
        });
    }
}

void Particles::update(float dt) {
    for (auto& p : pool) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.f) { p.active = false; continue; }
        p.x += p.vx * dt; p.y += p.vy * dt;
        p.vy += 260.f * dt;
        p.vx *= (1.f - 1.4f * dt);
    }
    pool.erase(std::remove_if(pool.begin(), pool.end(), [](const Particle& p) { return !p.active; }), pool.end());
}

void Particles::draw(sf::RenderTarget& rt, const Camera& cam) {
    for (const auto& p : pool) {
        float t = std::clamp(p.life / p.maxLife, 0.f, 1.f);
        sf::Color c = p.col;
        c.a = (std::uint8_t)(t * 255.f);
        gfx::fillCircle(rt, {cam.wx(p.x), cam.wy(p.y)}, p.size * (0.5f + t * 0.5f), c, 8);
    }
}
