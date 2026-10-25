#pragma once
// ═══════════════════════════════════════════
// ROBOT — Motobug (enemy) / Caterkiller (ally)
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <string>
#include "Physics.hpp"
#include "IDamageable.hpp"
#include "Camera.hpp"

enum class RobotType { Enemy, Ally };

class Robot {
public:
    Robot(float x, float y, RobotType type);

    void update(float dt, const std::vector<Platform>& plats, IDamageable* target);
    void takeDmg(float a);
    void draw(sf::RenderTarget& rt, const Camera& cam);

    float x, y, vx = 0.f, vy = 0.f;
    float w = 26.f, h = 34.f;
    bool onGround = false;
    static constexpr float GRAV = 900.f;

    RobotType type;
    float hp, maxHp;
    bool active = true;
    bool faceR = true;
    float atkCD = 0.f;
    std::string mode;

private:
    float patDir_;
    float patT_;
    float spd_;
    float wRot_ = 0.f;
    float segP_;
};
