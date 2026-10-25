#include "RescueTarget.hpp"
#include "Common.hpp"
#include <cmath>
#include <algorithm>
#include <cctype>

RescueTarget::RescueTarget(float startX, float startY, std::string n) : x(startX), y(startY), name(std::move(n)) {}

void RescueTarget::update(float dt) { t += dt; }

void RescueTarget::draw(sf::RenderTarget& rt, const Camera& cam, sf::Font& font) {
    if (!active) return;
    float cx = cam.wx(x), by = cam.wy(y);
    sf::Color col = (name == "Tails") ? gfx::hex("#ff9500") : gfx::hex("#cc2200");
    float pulse = 0.5f + 0.5f * std::sin(t * 4.5f);

    gfx::px(rt, cx - 13, by - h + h * 0.4f, w - 4, h * 0.5f, col);
    gfx::px(rt, cx - 11, by - h + h * 0.4f, w - 8, 4, name == "Tails" ? gfx::hex("#ffbb44") : gfx::hex("#ff4422"));
    gfx::fillCircle(rt, {cx, by - h + h * 0.2f}, w * 0.42f, col);

    for (int i = 0; i < 2; ++i) {
        float rp = std::fmod(pulse + (float)i * 0.5f, 1.f);
        sf::Color ringCol = col; ringCol.a = (std::uint8_t)(rp * 0.5f * 255.f);
        float radius = (w * 0.7f + (float)i * 12.f) * rp + 8.f;
        gfx::strokeCircle(rt, {cx, by - h * 0.55f}, radius, 2.f, ringCol);
    }

    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return std::toupper(c); });
    gfx::retroText(rt, font, upper, cx, by - h - 12, 11, sf::Color::White, sf::Color(0, 0, 0, 136), gfx::Align::Center);
    gfx::retroText(rt, font, "APPROACH TO RESCUE", cx, by - h - 25, 8, col, gfx::hex("#00000066"), gfx::Align::Center);
}
