#include "TailsDoll.hpp"
#include "Common.hpp"
#include <cmath>
#include <algorithm>

void TailsDoll::activateRuby() { rubyOn = true; }

void TailsDoll::update(float dt) {
    if (!active) return;
    floatT += dt; gemT += dt * 3.f;
    if (rubyOn) rubyPow = std::min(1.f, rubyPow + dt * 1.6f);
    if (followX_ && followY_) {
        float tx = *followX_ + 78.f, ty = *followY_ - 72.f + std::sin(floatT * 2.f) * 11.f;
        x += (tx - x) * 0.06f;
        y += (ty - y) * 0.06f;
    }
}

void TailsDoll::draw(sf::RenderTarget& rt, const Camera& cam) {
    if (!active) return;
    float cx = cam.wx(x), cy = cam.wy(y);

    gfx::fillEllipse(rt, {cx - 22, cy + 5}, 13.f, 6.f, gfx::hex("#ffe082"), -0.3f);
    gfx::fillEllipse(rt, {cx - 16, cy + 15}, 11.f, 5.f, gfx::hex("#ffe082"), 0.45f);
    gfx::px(rt, cx - 14, cy - 14, 28, 28, gfx::hex("#fffde7"));
    gfx::px(rt, cx - 12, cy - 14, 24, 4, gfx::hex("#ffffff"));
    gfx::px(rt, cx - 12, cy - 22, 8, 10, gfx::hex("#fff8e1"));
    gfx::px(rt, cx + 4, cy - 22, 8, 10, gfx::hex("#fff8e1"));
    gfx::px(rt, cx - 10, cy - 20, 4, 6, gfx::hex("#ffccaa"));
    gfx::px(rt, cx + 6, cy - 20, 4, 6, gfx::hex("#ffccaa"));
    gfx::px(rt, cx - 6, cy - 4, 5, 5, gfx::hex("#000000"));
    gfx::px(rt, cx + 1, cy - 4, 5, 5, gfx::hex("#000000"));
    gfx::px(rt, cx - 5, cy - 5, 2, 2, gfx::hex("#ffffff"));
    gfx::px(rt, cx + 2, cy - 5, 2, 2, gfx::hex("#ffffff"));

    sf::Color gc = rubyOn ? gfx::hsl(270.f + std::sin(gemT) * 45.f, 1.f, 0.62f) : gfx::hex("#ff1744");
    {
        sf::ConvexShape gem(4);
        gem.setPoint(0, {cx + 5, cy - 6});
        gem.setPoint(1, {cx + 13, cy - 13});
        gem.setPoint(2, {cx + 5, cy - 20});
        gem.setPoint(3, {cx - 3, cy - 13});
        gem.setFillColor(gc);
        rt.draw(gem);
    }
    {
        sf::ConvexShape facet(3);
        facet.setPoint(0, {cx + 5, cy - 16});
        facet.setPoint(1, {cx + 11, cy - 13});
        facet.setPoint(2, {cx + 5, cy - 11});
        facet.setFillColor(gfx::hexA("#ffffff", 0.27f));
        rt.draw(facet);
    }
    {
        float a = 0.4f + 0.3f * std::sin(gemT);
        sf::Color glowCol = gc; glowCol.a = (std::uint8_t)(a * 255.f);
        gfx::fillCircle(rt, {cx + 5, cy - 13}, 5.f, glowCol);
    }
    if (rubyOn && rubyPow > 0.15f) {
        sf::Color burst = gfx::hex("#ff00ff");
        burst.a = (std::uint8_t)(rubyPow * 0.38f * 255.f);
        gfx::fillCircle(rt, {cx, cy}, 48.f * rubyPow, burst, 32);
    }
}
