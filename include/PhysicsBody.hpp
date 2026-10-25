#pragma once
// ═══════════════════════════════════════════
// PHYSICS BODY
//
// Velocity/acceleration/friction/gravity, tuned to feel like a machine
// rather than a coordinate being teleported around:
//   - Horizontal movement ramps up through a short "thruster spool-up"
//     instead of snapping straight to full accel, then decays back down
//     fast once input releases - reads as thrusters spinning up/down
//     rather than an instant on/off platformer toggle.
//   - Friction is a flat deceleration (not exponential decay), so heavier
//     profiles visibly coast farther before stopping - the "can't turn on
//     a dime" weight cue.
//   - `mass` divides accel and impulses (F = ma), so a single knob makes
//     a character feel lighter/heavier without retuning every constant.
//
// Field names (x,y,vx,vy,w,h,onGround) intentionally match what
// Physics.hpp's resolveAABB<Body>() duck-types, so this body plugs into
// that function unchanged - call body.update(dt) then resolveAABB(body,
// plats) as two sequential calls, same as every entity in the existing
// project already does.
// ═══════════════════════════════════════════
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>

struct PhysicsProfile {
    float accelGround = 1400.f;      // px/s^2 at full thruster spool, on ground
    float accelAir = 700.f;          // px/s^2 in the air (reduced air control)
    float decelGround = 2200.f;      // px/s^2 friction when no input, on ground
    float decelAir = 260.f;          // px/s^2 friction when no input, airborne
    float maxSpeed = 260.f;          // px/s horizontal cap
    float mass = 1.f;                // divides accel & impulses - higher = heavier
    float gravity = 1400.f;          // px/s^2 downward
    float maxFallSpeed = 900.f;      // terminal velocity
    float jumpVelocity = -480.f;     // negative = upward; applied once on jump()
    float thrusterSpoolTime = 0.18f; // seconds from standstill to full thrust
    float gravityScale = 1.f;        // runtime-adjustable (e.g. Overdrive hover)
};

class PhysicsBody {
public:
    explicit PhysicsBody(PhysicsProfile profile = {}) : profile_(profile) {}

    void setInputDir(float dir) { inputDir_ = std::clamp(dir, -1.f, 1.f); }
    void jump();                          // no-op unless onGround
    void cutJumpShort();                  // variable jump height on early release
    void applyImpulse(sf::Vector2f imp);  // instant dV = imp / mass

    // Integrates vx/vy AND x/y for this frame. Caller runs resolveAABB()
    // immediately after (this function knows nothing about platforms).
    void update(float dt);

    PhysicsProfile& profile() { return profile_; }
    const PhysicsProfile& profile() const { return profile_; }

    // vy carried INTO this frame, before this frame's gravity/collision.
    // After resolveAABB runs, `if (onGround && lastVerticalSpeed() > N)`
    // is a hard landing worth a camera shake / clank sound / dust puff.
    float lastVerticalSpeed() const { return lastVy_; }
    float thrusterSpool() const { return thrusterSpool_; } // 0..1, for VFX (engine glow intensity)

    // Duck-typed fields required by resolveAABB<Body>().
    float x = 0.f, y = 0.f;
    float vx = 0.f, vy = 0.f;
    float w = 32.f, h = 45.f;
    bool onGround = false;

private:
    PhysicsProfile profile_;
    float inputDir_ = 0.f;
    float thrusterSpool_ = 0.f;
    float lastVy_ = 0.f;
};
