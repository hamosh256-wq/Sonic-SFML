#pragma once
// ═══════════════════════════════════════════
// SONIC.EXE — boss entity
//
// ADAPTED for the unified project: `p_` used to be a `MetalSonic*`
// (the old procedural class); it's now a `MetalSonicController*`, and
// every place that used to read `p->x`/`p->y`/`p->active` directly now
// goes through the IDamageable interface (`posX()`/`posY()`/
// `isActive()`) that both the old and new player classes implement -
// same pattern Robot.cpp already uses for its own target-tracking.
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <vector>
#include "StateMachine.hpp"
#include "EventBus.hpp"
#include "Physics.hpp"
#include "DrRobotnik.hpp"
#include "Camera.hpp"
#include "MetalSonicController.hpp"
#include "IDamageable.hpp"

// Also implements IDamageable (in addition to being MetalSonicController's
// opponent) so that ally Robots - which target through the same interface
// Robot.cpp already uses for the player - can attack Sonic.exe during the
// boss phases. Without this, allies in "attack" mode have no legal way to
// damage the boss at all.
class SonicExe : public IDamageable {
public:
    SonicExe(float startX, float startY);

    void setPhase(int p);
    void takeDmg(float a);
    void update(float dt, MetalSonicController* player, DrRobotnik* robotnik, EventBus& ev,
                const std::vector<Platform>& plats);
    void draw(sf::RenderTarget& rt, const Camera& cam);

    // ── IDamageable ──
    float posX() const override { return x; }
    float posY() const override { return y; }
    bool isActive() const override { return active && !victoryMode; }
    void takeDamage(float amount) override { takeDmg(amount); }

    StateMachine sm;

    float x, y, vx = 0.f, vy = 0.f;
    float w = 34.f, h = 78.f;
    bool faceR = true;
    bool onGround = false;
    float hp = 250.f, maxHp = 250.f;
    float iframes = 0.f;
    bool active = true;
    bool victoryMode = false;
    int phase = 1;
    float aurT = 0.f;

    static constexpr float GRAV = 900.f;

private:
    void grav(float dt);

    MetalSonicController* p_ = nullptr;
    DrRobotnik* r_ = nullptr;
    EventBus* e_ = nullptr;
    bool lunge_ = false;

    // Curse exposure dealt to the player alongside the physical lunge hit
    // (see the Attacking state's update lambda in SonicExe.cpp) - this is
    // the NEW integration point: every reality-warping attack now feeds
    // MetalSonicController's Curse meter, not just its HP.
    static constexpr float CURSE_HIT_PHASE1 = 12.f;
    static constexpr float CURSE_HIT_PHASE2 = 20.f;
};
