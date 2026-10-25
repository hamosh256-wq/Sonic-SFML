#pragma once
// ═══════════════════════════════════════════
// IDamageable — identical to the one already in your project.
// ═══════════════════════════════════════════

class IDamageable {
public:
    virtual float posX() const = 0;
    virtual float posY() const = 0;
    virtual bool isActive() const = 0;
    virtual void takeDamage(float amount) = 0;
    virtual ~IDamageable() = default;
};
