#include "Robot.hpp"
#include "Common.hpp"
#include "Clock.hpp"
#include <cmath>
#include <algorithm>

Robot::Robot(float startX, float startY, RobotType t) : type(t) {
    x = startX; y = startY;
    hp = (type == RobotType::Enemy) ? 32.f : 26.f;
    maxHp = hp;
    mode = (type == RobotType::Enemy) ? "patrol" : "defend";
    patDir_ = gfx::frand() > 0.5f ? 1.f : -1.f;
    patT_ = gfx::frand() * 2.5f;
    spd_ = 72.f + gfx::frand() * 52.f;
    segP_ = gfx::frand() * 3.14159265f * 2.f;
}

void Robot::update(float dt, const std::vector<Platform>& plats, IDamageable* target) {
    if (!active) return;
    atkCD -= dt; patT_ += dt;
    if (!onGround) vy = std::min(vy + GRAV * dt, 700.f);

    if (type == RobotType::Enemy) {
        if (target && std::hypot(target->posX() - x, target->posY() - y) < 290.f) {
            float dx = target->posX() - x;
            vx = gfx::sign(dx) * spd_ * 1.35f; faceR = dx > 0.f;
            if (std::fabs(dx) < 52.f && atkCD <= 0.f) { target->takeDamage(8.f); atkCD = 2.1f; }
        } else {
            if (patT_ > 2.8f) { patDir_ *= -1.f; patT_ = 0.f; }
            vx = patDir_ * spd_ * 0.58f; faceR = vx > 0.f;
        }
    } else {
        if (target && target->isActive() && mode == "attack") {
            float dx = target->posX() - x;
            vx = gfx::sign(dx) * spd_ * 1.6f; faceR = dx > 0.f;
            if (std::fabs(dx) < 58.f && atkCD <= 0.f) { target->takeDamage(9.f); atkCD = 2.0f; }
        } else {
            vx *= 0.78f;
        }
    }

    x += vx * dt; y += vy * dt;
    wRot_ += (vx / 30.f) * dt;
    resolveAABB(*this, plats);
}

void Robot::takeDmg(float a) {
    hp -= a;
    if (hp <= 0.f) active = false;
}

void Robot::draw(sf::RenderTarget& rt, const Camera& cam) {
    if (!active) return;
    float cx = cam.wx(x), by = cam.wy(y);
    float t = clk::now();

    if (type == RobotType::Enemy) {
        sf::Vector2f wheelCenter{cx, by - 6};
        sf::RectangleShape wheelBody({18.f, 18.f});
        wheelBody.setOrigin({9.f, 9.f});
        wheelBody.setPosition(wheelCenter);
        wheelBody.setRotation(sf::radians(wRot_));
        wheelBody.setFillColor(gfx::hex("#1a1a1a"));
        rt.draw(wheelBody);

        for (int i = 0; i < 4; ++i) {
            float a = (float)i * 3.14159265f / 2.f;
            sf::Vector2f local = gfx::rotatePoint({std::cos(a) * 8.f, std::sin(a) * 8.f}, wRot_);
            gfx::thickLine(rt, wheelCenter, wheelCenter + local, 2.f, gfx::hex("#333333"));
        }
        sf::RectangleShape hub({6.f, 6.f});
        hub.setOrigin({3.f, 3.f});
        hub.setPosition(wheelCenter);
        hub.setRotation(sf::radians(wRot_));
        hub.setFillColor(gfx::hex("#444444"));
        rt.draw(hub);

        gfx::px(rt, cx - w * 0.46f, by - h * 0.66f, w * 0.92f, h * 0.56f, gfx::hex("#8b0000"));
        gfx::px(rt, cx - w * 0.44f, by - h * 0.66f, w * 0.88f, 4, gfx::hex("#aa1111"));
        gfx::thickLine(rt, {cx - w * 0.46f, by - h * 0.42f}, {cx + w * 0.46f, by - h * 0.42f}, 1.5f, gfx::hex("#660000"));
        gfx::thickLine(rt, {cx, by - h * 0.66f}, {cx, by - h * 0.1f}, 1.5f, gfx::hex("#660000"));
        float eX = faceR ? cx + 3.f : cx - w * 0.42f;
        gfx::px(rt, eX, by - h * 0.52f, 10, 10, gfx::hex("#ff0000"));
        gfx::px(rt, eX + 1, by - h * 0.52f + 1, 4, 4, gfx::hex("#ff8888"));
        gfx::px(rt, eX + 3, by - h * 0.52f + 4, 4, 4, gfx::hex("#110000"));
        gfx::px(rt, cx - 4, by - h * 0.66f, 2, 8, gfx::hex("#660000"));
        gfx::px(rt, cx + 2, by - h * 0.66f, 2, 6, gfx::hex("#660000"));
        gfx::px(rt, cx - 6, by - h * 0.66f - 6, 4, 4, gfx::hex("#ff3300"));
        gfx::px(rt, cx + 2, by - h * 0.66f - 4, 4, 4, gfx::hex("#ff3300"));
    } else {
        float dir = faceR ? 1.f : -1.f;
        for (int i = 3; i >= 0; --i) {
            float sx = cx + dir * (-(float)i * 9.f);
            float wave = std::sin(segP_ + t * 4.f + (float)i * 0.8f) * 3.f;
            float sr = 9.f - (float)i * 1.5f;
            sf::Color col = (i == 0) ? gfx::hex("#006622") : gfx::hex("#004411");
            gfx::fillCircle(rt, {sx, by - h * 0.45f + wave}, sr, col);
            gfx::fillCircle(rt, {sx - sr * 0.3f, by - h * 0.45f + wave - sr * 0.3f}, sr * 0.4f, gfx::hex("#008833"));
        }
        float hX = cx + dir * 18.f;
        gfx::px(rt, hX - 10, by - h * 0.65f, 20, 20, gfx::hex("#006622"));
        gfx::px(rt, hX - 8, by - h * 0.65f, 16, 4, gfx::hex("#008833"));
        gfx::px(rt, hX - 5, by - h * 0.55f, 6, 6, gfx::hex("#00ff66"));
        gfx::px(rt, hX + 1, by - h * 0.55f, 6, 6, gfx::hex("#00ff66"));
        gfx::px(rt, hX - 4, by - h * 0.53f, 3, 3, gfx::hex("#ffffff"));
        gfx::px(rt, hX + 2, by - h * 0.53f, 3, 3, gfx::hex("#ffffff"));
    }

    if (hp < maxHp) {
        float bw = w + 6.f;
        gfx::px(rt, cx - bw / 2.f, by - h - 8, bw, 4, gfx::hex("#000000"));
        gfx::px(rt, cx - bw / 2.f, by - h - 8, bw * (hp / maxHp), 4,
                type == RobotType::Enemy ? gfx::hex("#ff3333") : gfx::hex("#00ff44"));
    }
}
