#pragma once
// ═══════════════════════════════════════════
// METAL SONIC CONTROLLER
//
// The playable character. Composition, not inheritance, over the three
// systems above: a PhysicsBody for movement, an AnimationController for
// the sprite sheet, and a CurseState for the corruption meter. A
// StateMachine (the exact same class used everywhere else in your
// project) governs the transformation arc:
//
//     Normal --(hold F, curse charged)--> Charging
//     Charging --(held long enough)-->   Super   --(released early)--> Normal
//     Super --(hold F again, vs .exe, curse refilled)--> Hyper
//
// Hyper is only reachable while vsEXE_ is true (set by Game during the
// boss fight), which makes it structurally "the anti-.exe form" rather
// than a generic late-game power-up - entering it always means the fight
// that follows is the one the corruption sequence cares about.
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "PhysicsBody.hpp"
#include "SpriteAnimation.hpp"
#include "CurseSystem.hpp"
#include "StateMachine.hpp"
#include "EventBus.hpp"
#include "IDamageable.hpp"
#include "Input.hpp"
#include "Physics.hpp" // Platform

class MetalSonicController : public IDamageable {
public:
    MetalSonicController(const sf::Texture& texture, float startX, float startY, EventBus& ev);

    // Respawns in place rather than being reconstructed - this class holds
    // an EventBus& reference member, so it is deliberately NOT copy/move-
    // assignable (that's a feature, not an oversight: a reference member
    // can't be silently rebound to a different EventBus mid-game). Use
    // this instead of `metal = MetalSonicController(...)`.
    void reset(float x, float y);

    void update(float dt, const Input& input, const std::vector<Platform>& plats);
    void draw(sf::RenderTarget& rt) const { rt.draw(anim_.sprite()); }

    // ── IDamageable: ordinary physical hits (rings) ──
    float posX() const override { return body_.x; }
    float posY() const override { return body_.y; }
    bool isActive() const override { return active_; }
    void takeDamage(float amount) override;

    // Reality-warping hits from Sonic.exe go through the Curse system
    // instead - call this from SonicExe's attack resolution, separately
    // from takeDamage().
    void exposeToCorruption(float amount) { curse_.exposeToCorruption(amount); }

    // Game calls this true/false as Metal Sonic enters/leaves a fight
    // with the .exe entity (e.g. on boss1/boss2 phase transitions).
    void setFacingCorruptionEntity(bool v) { vsEXE_ = v; }

    const std::string& formState() const { return sm_.current(); }
    float rings() const { return hp_; }
    float maxRings() const { return maxHp_; }
    float curse01() const { return curse_.corruption01(); }
    bool isGlitched() const { return glitched_; }
    bool facingRight() const { return faceR_; }

    // NEW for the unified project: MetalSonicController as originally
    // scoped only covered movement/transformation, with no combat signal
    // at all - but the existing enemy (Robot) and boss (SonicExe) combat
    // loops both need one (`isAttacking()` + a damage value) to resolve
    // hits against the player. Airborne or Super/Hyper counts as an
    // active hitbox - matches Metal Sonic's flight-capable moveset,
    // where he's the aggressor whenever he's off the ground rather than
    // needing a dedicated attack button.
    bool isAttacking() const { return !body_.onGround || sm_.any({"Super", "Hyper"}); }
    float atk() const {
        if (sm_.is("Hyper")) return 40.f;
        if (sm_.is("Super")) return 26.f;
        return 14.f;
    }

    // Lets Game apply the camera's screen-space offset right before
    // drawing, without disturbing the world-space position update()
    // maintains internally (that world-space auto-positioning is what
    // lets the standalone demo in main_demo.cpp work with zero camera
    // code at all - this is an additive override, not a replacement).
    void setRenderPosition(sf::Vector2f screenPos) { anim_.setPosition(screenPos); }

    PhysicsBody& body() { return body_; }
    const PhysicsBody& body() const { return body_; }

private:
    void buildStates();
    void pickAnimation(); // chooses/plays the right clip for physics + form state

    PhysicsBody body_;
    AnimationController anim_;
    CurseState curse_;
    StateMachine sm_;
    EventBus& ev_;

    bool active_ = true;
    bool faceR_ = true;
    float hp_ = 100.f, maxHp_ = 100.f;
    bool vsEXE_ = false;

    bool glitched_ = false;
    float glitchTimer_ = 0.f;
    float chargeHeld_ = 0.f;
    const Input* curInput_ = nullptr; // set at the top of update(), read by state lambdas
    bool jumpHeldPrev_ = false;       // for detecting release -> cutJumpShort()

    static constexpr float CHARGE_THRESHOLD = 40.f;   // curse needed to START charging
    static constexpr float HOLD_TO_TRANSFORM = 0.6f;  // seconds F must be held to complete charge
    static constexpr float SUPER_DRAIN_PER_SEC = 8.f;  // curse-fuel burned per second in Super
    static constexpr float HYPER_REQUIRES_CURSE = 70.f; // curse needed (while in Super) to reach Hyper
    static constexpr float GLITCH_DURATION = 1.2f;
    static constexpr float RUN_SPEED_THRESHOLD = 140.f; // px/s - above this, play Run instead of Walk
};
