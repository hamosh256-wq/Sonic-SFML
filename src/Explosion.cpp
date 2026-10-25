#include "Explosion.hpp"
#include "Common.hpp"
#include <cmath>
#include <algorithm>

Explosion::Explosion(float x, float y, float scale) : x_(x), y_(y), scale_(scale) {
    int n = 8 + (int)(gfx::frand() * 5.f);
    for (int i = 0; i < n; ++i) {
        float a = gfx::frand() * 3.14159265f * 2.f;
        float s = (60.f + gfx::frand() * 140.f) * scale_;
        debris_.push_back(Debris{
            0.f, 0.f, std::cos(a) * s, std::sin(a) * s - 60.f,
            gfx::frand() * 6.28f, (gfx::frand() - 0.5f) * 10.f,
            (2.f + gfx::frand() * 4.f) * scale_
        });
    }
}

void Explosion::update(float dt) {
    t_ += dt;
    if (t_ >= dur_) { active = false; return; }
    for (auto& d : debris_) {
        d.x += d.vx * dt; d.y += d.vy * dt;
        d.vy += 340.f * dt;
        d.rot += d.rotSpd * dt;
    }
}

void Explosion::draw(sf::RenderTarget& rt, const Camera& cam) {
    float t = std::clamp(t_ / dur_, 0.f, 1.f);
    float cx = cam.wx(x_), cy = cam.wy(y_);

    // Expanding shockwave ring
    if (t < 0.5f) {
        float rt2 = t / 0.5f;
        gfx::strokeCircle(rt, {cx, cy}, 10.f + rt2 * 46.f * scale_, 3.f * (1.f - rt2),
                          gfx::rgba(255, 200, 80, 1.f - rt2));
    }

    // Core flash
    if (t < 0.25f) {
        float ft = 1.f - (t / 0.25f);
        gfx::fillCircle(rt, {cx, cy}, 18.f * scale_ * ft, gfx::rgba(255, 240, 200, ft));
    }

    // Debris chunks
    for (const auto& d : debris_) {
        float a = 1.f - t;
        sf::RectangleShape r({d.size, d.size});
        r.setOrigin({d.size / 2.f, d.size / 2.f});
        r.setPosition({cx + d.x, cy + d.y});
        r.setRotation(sf::radians(d.rot));
        r.setFillColor(gfx::rgba(255, 140 + (int)(gfx::frand() * 60.f), 40, a));
        rt.draw(r);
    }
}
