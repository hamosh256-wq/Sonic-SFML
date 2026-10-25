#include "DrRobotnik.hpp"
#include "Common.hpp"
#include <algorithm>

DrRobotnik::DrRobotnik(float startX, float startY) : x(startX), y(startY) {}

void DrRobotnik::takeDmg(float a) {
    if (iframes > 0.f || !active) return;
    hp = std::max(0.f, hp - a);
    iframes = 0.5f;
    if (hp <= 0.f) active = false;
}

void DrRobotnik::update(float dt, EventBus& ev) {
    if (iframes > 0.f) iframes -= dt;
    if (!active) return;

    // Periodically calls in an ally Robot as reinforcement. Game::wireEvents
    // only actually spawns one while a boss phase is running, so it's safe
    // for this to just fire on a fixed cycle regardless of context.
    rt_ += dt;
    if (rt_ > 9.f) {
        rt_ = 0.f;
        ev.emit("reinforce", {x, y});
    }
}

void DrRobotnik::draw(sf::RenderTarget& rt, const Camera& cam) {
    if (!active) return;
    float cx = cam.wx(x), by = cam.wy(y);
    float w_ = w, h_ = h;

    bool hit = iframes > 0.f;
    sf::Color bodyCol = hit ? gfx::hex("#ff8888") : gfx::hex("#cc2222");
    sf::Color shadeCol = hit ? gfx::hex("#ffaaaa") : gfx::hex("#ee4444");

    // Legs / boots
    gfx::px(rt, cx - w_ * 0.4f, by - 10, w_ * 0.32f, 10, gfx::hex("#5b0000"));
    gfx::px(rt, cx + w_ * 0.08f, by - 10, w_ * 0.32f, 10, gfx::hex("#5b0000"));

    // Egg-shaped torso
    gfx::fillEllipse(rt, {cx, by - h_ * 0.42f}, w_ * 0.46f, h_ * 0.4f, bodyCol);
    gfx::fillEllipse(rt, {cx, by - h_ * 0.5f}, w_ * 0.3f, h_ * 0.16f, shadeCol);
    gfx::px(rt, cx - 3, by - h_ * 0.62f, 6, h_ * 0.5f, gfx::hex("#8b0000"));

    // Arms
    gfx::fillEllipse(rt, {cx - w_ * 0.52f, by - h_ * 0.4f}, 7.f, 14.f, bodyCol, -0.3f);
    gfx::fillEllipse(rt, {cx + w_ * 0.52f, by - h_ * 0.4f}, 7.f, 14.f, bodyCol, 0.3f);
    gfx::fillCircle(rt, {cx - w_ * 0.6f, by - h_ * 0.28f}, 5.f, gfx::hex("#5b0000"));
    gfx::fillCircle(rt, {cx + w_ * 0.6f, by - h_ * 0.28f}, 5.f, gfx::hex("#5b0000"));

    // Head
    gfx::fillCircle(rt, {cx, by - h_ * 0.82f}, w_ * 0.34f, gfx::hex("#ffcc99"));
    gfx::px(rt, cx - w_ * 0.36f, by - h_ - 4.f, w_ * 0.72f, 10.f, gfx::hex("#8b0000"));

    // Goggles
    gfx::fillCircle(rt, {cx - 7.f, by - h_ * 0.84f}, 6.5f, gfx::hex("#111111"));
    gfx::fillCircle(rt, {cx + 7.f, by - h_ * 0.84f}, 6.5f, gfx::hex("#111111"));
    gfx::fillCircle(rt, {cx - 7.f, by - h_ * 0.84f}, 3.f, gfx::hex("#66ccff"));
    gfx::fillCircle(rt, {cx + 7.f, by - h_ * 0.84f}, 3.f, gfx::hex("#66ccff"));

    // Mustache
    gfx::fillEllipse(rt, {cx - w_ * 0.18f, by - h_ * 0.63f}, 9.f, 5.f, gfx::hex("#f57f17"), -0.2f);
    gfx::fillEllipse(rt, {cx + w_ * 0.18f, by - h_ * 0.63f}, 9.f, 5.f, gfx::hex("#f57f17"), 0.2f);

    if (hp < maxHp) {
        float bw = w_ + 10.f;
        gfx::px(rt, cx - bw / 2.f, by - h_ - 20, bw, 5, gfx::hex("#000000"));
        gfx::px(rt, cx - bw / 2.f, by - h_ - 20, bw * (hp / maxHp), 5, gfx::hex("#cc66ff"));
    }
}
