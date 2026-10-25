#include "PhysicsBody.hpp"

void PhysicsBody::update(float dt) {
    const float accel = onGround ? profile_.accelGround : profile_.accelAir;
    const float decel = onGround ? profile_.decelGround : profile_.decelAir;

    if (std::fabs(inputDir_) > 0.001f) {
        // Spool toward full thrust rather than snapping to it - this is
        // the single biggest contributor to the "mechanical" feel; a
        // linear/instant accel model reads as a coordinate being dragged,
        // not a machine spinning up.
        thrusterSpool_ = std::min(1.f, thrusterSpool_ + dt / std::max(0.001f, profile_.thrusterSpoolTime));
        float spooledAccel = (accel * (0.35f + 0.65f * thrusterSpool_)) / profile_.mass;
        vx += inputDir_ * spooledAccel * dt;
        vx = std::clamp(vx, -profile_.maxSpeed, profile_.maxSpeed);
    } else {
        // Thrusters wind down faster than they wind up (3x rate) - avoids
        // a mushy, floaty stop while still not being an instant halt.
        thrusterSpool_ = std::max(0.f, thrusterSpool_ - dt * 3.f);
        float d = (decel * dt) / profile_.mass;
        if (vx > 0.f) vx = std::max(0.f, vx - d);
        else if (vx < 0.f) vx = std::min(0.f, vx + d);
    }

    lastVy_ = vy; // snapshot BEFORE this frame's gravity - see header comment
    if (!onGround) {
        vy += profile_.gravity * profile_.gravityScale * dt;
        vy = std::min(vy, profile_.maxFallSpeed);
    }

    x += vx * dt;
    y += vy * dt;
}

void PhysicsBody::jump() {
    if (!onGround) return;
    vy = profile_.jumpVelocity;
    onGround = false;
}

void PhysicsBody::cutJumpShort() {
    if (vy < 0.f) vy *= 0.45f; // still moving upward -> clip the arc short
}

void PhysicsBody::applyImpulse(sf::Vector2f imp) {
    vx += imp.x / profile_.mass;
    vy += imp.y / profile_.mass;
}
