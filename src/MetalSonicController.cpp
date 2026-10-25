#include "MetalSonicController.hpp"
#include "SpriteData.hpp"
#include <unordered_map>
#include <algorithm>
#include <cmath>

MetalSonicController::MetalSonicController(const sf::Texture& texture, float startX, float startY, EventBus& ev)
    : anim_(texture), curse_(100.f, 1.0f), ev_(ev) {
    // Heavier-than-default profile + a visible thruster spool: this is
    // what "mechanical and weighty" actually cashes out to in numbers.
    // Metal Sonic accelerates like something with real inertia behind it
    // rather than snapping to top speed, but once moving, low ground
    // friction lets him coast/slide - a machine, not a rubber ball.
    PhysicsProfile p;
    p.mass = 1.35f;
    p.accelGround = 1550.f;
    p.accelAir = 620.f;
    p.decelGround = 1100.f;   // lower than accel -> visibly slides to a stop
    p.decelAir = 180.f;
    p.maxSpeed = 300.f;
    p.gravity = 1500.f;
    p.maxFallSpeed = 950.f;
    p.jumpVelocity = -520.f;
    p.thrusterSpoolTime = 0.22f;
    body_ = PhysicsBody(p);
    body_.x = startX; body_.y = startY;
    body_.w = 34.f; body_.h = 45.f; // matches the Idle clip's typical frame size

    for (const auto& [name, frames] : spritedata::metalSonicFrames()) {
        float dur = (name == "Damaged_EMP") ? 0.09f : 0.07f; // glitch flicker reads faster
        anim_.addClip(name, SpriteAnimation(frames, dur, /*loop=*/true));
    }
    anim_.play("Idle");

    buildStates();
}

// ═══════════════════════════════════════════ STATE MACHINE ═══════════════════════════════════════════

void MetalSonicController::buildStates() {
    MetalSonicController* S = this;
    std::unordered_map<std::string, StateDef> states;

    states["Normal"] = StateDef{
        nullptr, nullptr,
        [S](float dt, float) {
            if (S->glitched_) {
                S->glitchTimer_ -= dt;
                if (S->glitchTimer_ <= 0.f) S->glitched_ = false;
                return; // no transform input while glitched
            }
            if (S->curse_.isOverwhelmed()) {
                S->glitched_ = true;
                S->glitchTimer_ = GLITCH_DURATION;
                S->ev_.emit("curse_overwhelmed", {S->body_.x, S->body_.y});
                return;
            }
            if (S->curInput_ && S->curInput_->down(GKey::F) && S->curse_.canChannel(CHARGE_THRESHOLD)) {
                S->sm_.go("Charging");
            }
        }};

    states["Charging"] = StateDef{
        [S]() {
            S->chargeHeld_ = 0.f;
            S->ev_.emit("phantom_ruby_activate", {S->body_.x, S->body_.y});
        },
        nullptr,
        [S](float dt, float) {
            if (!S->curInput_ || !S->curInput_->down(GKey::F)) {
                S->sm_.go("Normal"); // released early -> Ruby deactivates, no refund
                return;
            }
            if (S->curse_.isOverwhelmed() && S->chargeHeld_ < 0.05f) {
                // Getting hit hard enough to overwhelm right as charging
                // starts knocks it straight into a glitch instead of
                // letting the Ruby stabilize.
                S->glitched_ = true; S->glitchTimer_ = GLITCH_DURATION;
                S->sm_.go("Normal");
                return;
            }
            S->chargeHeld_ += dt;
            if (S->chargeHeld_ >= HOLD_TO_TRANSFORM) {
                S->curse_.vent(CHARGE_THRESHOLD);
                S->sm_.go("Super");
            }
        }};

    states["Super"] = StateDef{
        [S]() { S->ev_.emit("phantom_ruby_super", {S->body_.x, S->body_.y}); },
        nullptr,
        [S](float dt, float) {
            float vented = S->curse_.vent(SUPER_DRAIN_PER_SEC * dt);
            if (vented <= 0.f) { S->sm_.go("Normal"); return; } // ran out of fuel
            if (S->vsEXE_ && S->curInput_ && S->curInput_->down(GKey::F) &&
                S->curse_.canChannel(HYPER_REQUIRES_CURSE)) {
                S->sm_.go("Hyper");
            }
        }};

    states["Hyper"] = StateDef{
        [S]() {
            S->curse_.vent(S->curse_.corruption()); // spend it all - point of no return
            S->ev_.emit("phantom_ruby_hyper", {S->body_.x, S->body_.y});
            // Reachable ONLY while vsEXE_ (guarded in Super's update above),
            // so arriving here at all IS "Hyper Form against the .exe" -
            // this is the one event CorruptionDirector subscribes to.
            // See CorruptionDirector.hpp for why what follows is a
            // scripted in-game illusion rather than touching any real
            // file on disk.
            S->ev_.emit("hyper_vs_exe", {S->body_.x, S->body_.y});
        },
        nullptr, nullptr};

    sm_.init(std::move(states), "Normal");
}

// ═══════════════════════════════════════════ UPDATE ═══════════════════════════════════════════

void MetalSonicController::update(float dt, const Input& input, const std::vector<Platform>& plats) {
    if (!active_) return;
    curInput_ = &input;
    curse_.update(dt);

    // Movement input applies in every form state - only the transformation
    // logic above branches on state. Glitched locks movement out entirely
    // (see Normal's update lambda) by never reaching this if we return
    // early... but movement should still respond even while glitched-and-
    // idle-standing per design intent ("systems overwhelmed" reads better
    // as sluggish/unresponsive than as a hard stun), so we deliberately
    // do NOT gate this block on glitched_ - only on active_.
    float dir = 0.f;
    if (input.down(GKey::Left) || input.down(GKey::A)) dir -= 1.f;
    if (input.down(GKey::Right) || input.down(GKey::D)) dir += 1.f;
    if (glitched_) dir *= 0.25f; // sluggish, not frozen
    body_.setInputDir(dir);
    if (dir > 0.01f) faceR_ = true;
    else if (dir < -0.01f) faceR_ = false;

    bool jumpDown = input.down(GKey::Space) || input.down(GKey::Up) || input.down(GKey::W);
    if (jumpDown && !jumpHeldPrev_) body_.jump();
    if (!jumpDown && jumpHeldPrev_) body_.cutJumpShort();
    jumpHeldPrev_ = jumpDown;

    sm_.tick(dt);

    float vyBefore = body_.lastVerticalSpeed();
    body_.update(dt);
    resolveAABB(body_, plats);
    if (body_.onGround && vyBefore > 620.f) {
        ev_.emit("hard_landing", {body_.x, body_.y});
    }

    pickAnimation();
    anim_.setFacingRight(faceR_);
    anim_.update(dt);
    anim_.setPosition({body_.x, body_.y});

    // Cosmetic form tint - the sheet has no dedicated Super/Hyper frames
    // (it's a partial rip), so the transformation reads through color and
    // FX rather than pretending frames exist that don't.
    if (sm_.is("Hyper"))      anim_.sprite().setColor(sf::Color(255, 210, 90));
    else if (sm_.is("Super")) anim_.sprite().setColor(sf::Color(140, 230, 255));
    else if (glitched_)       anim_.sprite().setColor(sf::Color(170, 170, 170));
    else                      anim_.sprite().setColor(sf::Color::White);

    curInput_ = nullptr;
}

void MetalSonicController::pickAnimation() {
    if (glitched_) { anim_.play("Damaged_EMP"); return; }
    if (!body_.onGround) { anim_.play("JumpSpin"); return; }
    if (sm_.any({"Super", "Hyper"})) { anim_.play("Dash_Boost"); return; }
    float speed = std::fabs(body_.vx);
    if (speed < 5.f) anim_.play("Idle");
    else if (speed < RUN_SPEED_THRESHOLD) anim_.play("Walk");
    else anim_.play("Run_B");
}

void MetalSonicController::reset(float x, float y) {
    body_.x = x; body_.y = y;
    body_.vx = 0.f; body_.vy = 0.f;
    body_.onGround = false;
    active_ = true;
    faceR_ = true;
    hp_ = maxHp_;
    vsEXE_ = false;
    glitched_ = false;
    glitchTimer_ = 0.f;
    chargeHeld_ = 0.f;
    jumpHeldPrev_ = false;
    curInput_ = nullptr;
    // Fresh meter, but keep whatever resistance/max tuning was set (in
    // case a difficulty setting or upgrade had customized them).
    curse_ = CurseState(curse_.maxCorruption(), curse_.resistance());
    sm_.go("Normal");
    anim_.play("Idle", /*restart=*/true);
}

void MetalSonicController::takeDamage(float amount) {
    if (!active_) return;
    hp_ = std::max(0.f, hp_ - amount);
    if (hp_ <= 0.f) active_ = false;
}
