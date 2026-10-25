#include "SonicExe.hpp"
#include "Common.hpp"
#include "Clock.hpp"
#include <unordered_map>
#include <cmath>
#include <algorithm>

SonicExe::SonicExe(float startX, float startY) {
    x = startX; y = startY;

    SonicExe* S = this;
    std::unordered_map<std::string, StateDef> states;

    states["Stalking"] = StateDef{nullptr, nullptr,
        [S](float dt, float t) {
            S->grav(dt);
            MetalSonicController* p = S->p_;
            if (!p || !p->isActive()) return;
            float dx = p->posX() - S->x, dist = std::fabs(dx);
            float spd = S->phase == 1 ? 155.f : 300.f;
            if (dist > 95.f) { S->vx = gfx::sign(dx) * spd; S->faceR = dx > 0.f; }
            else { S->vx *= 0.82f; if (t > 0.9f) S->sm.go("Attacking"); }
            if (t > (S->phase == 1 ? 5.5f : 3.2f)) S->sm.go("Teleporting");
            if (S->phase == 2 && S->r_ && S->r_->hp > 0.f && gfx::frand() < 0.007f) S->sm.go("Executing");
        }};

    states["Teleporting"] = StateDef{nullptr, nullptr,
        [S](float dt, float t) {
            S->vx = 0.f;
            if (t > 0.45f) {
                MetalSonicController* p = S->p_;
                if (p) {
                    float side = (gfx::frand() > 0.5f ? 1.f : -1.f) * (85.f + gfx::frand() * 130.f);
                    S->x = std::max(70.f, std::min(p->posX() + side, 1130.f));
                    S->y = p->posY() - 25.f; S->vy = 0.f;
                    if (S->e_) S->e_->emit("exe_teleport", {S->x, S->y});
                }
                S->sm.go("Attacking");
            }
        }};

    states["Attacking"] = StateDef{
        [S]() {
            S->lunge_ = true;
            MetalSonicController* p = S->p_;
            if (p) {
                float dx = p->posX() - S->x, dy = (p->posY() - 22.f) - S->y;
                float len = std::hypot(dx, dy); if (len == 0.f) len = 1.f;
                float spd = S->phase == 1 ? 570.f : 760.f;
                S->vx = (dx / len) * spd; S->vy = (dy / len) * 320.f - 75.f;
                S->faceR = S->vx > 0.f;
            }
        },
        nullptr,
        [S](float dt, float t) {
            S->grav(dt); S->vx *= 0.91f;
            if (S->lunge_ && S->p_) {
                float d = std::hypot(S->p_->posX() - S->x, (S->p_->posY() - 22.f) - S->y);
                if (d < 68.f) {
                    S->p_->takeDamage(S->phase == 1 ? 14.f : 24.f);
                    // NEW: every landed hit is a reality-warping one, so it
                    // also feeds the Curse meter - separate channel, same
                    // strike. See MetalSonicController::exposeToCorruption().
                    S->p_->exposeToCorruption(S->phase == 1 ? CURSE_HIT_PHASE1 : CURSE_HIT_PHASE2);
                    S->lunge_ = false;
                    if (S->e_) S->e_->emit("exe_hit_player");
                }
            }
            if (t > (S->phase == 1 ? 1.6f : 1.05f) || S->onGround) S->sm.go("Stalking");
        }};

    states["Executing"] = StateDef{nullptr, nullptr,
        [S](float dt, float t) {
            S->grav(dt);
            DrRobotnik* r = S->r_;
            if (!r || r->hp <= 0.f) { S->sm.go("Stalking"); return; }
            float dx = r->x - S->x;
            S->vx = gfx::sign(dx) * 400.f; S->faceR = dx > 0.f;
            if (std::fabs(dx) < 72.f) {
                r->takeDmg(30.f);
                if (S->e_) S->e_->emit("exe_hit_robotnik");
                S->sm.go("Stalking");
            }
            if (S->p_ && std::hypot(S->p_->posX() - S->x, S->p_->posY() - S->y) < 85.f) S->sm.go("Attacking");
            if (t > 3.5f) S->sm.go("Stalking");
        }};

    states["Victory"] = StateDef{
        [S]() { S->vx = 0.f; S->vy = 0.f; S->victoryMode = true; },
        nullptr, nullptr};

    sm.init(std::move(states), "Stalking");
}

void SonicExe::grav(float dt) {
    if (!onGround) vy = std::min(vy + GRAV * dt, 700.f);
}

void SonicExe::setPhase(int p) {
    phase = p;
    sm.go("Stalking");
}

void SonicExe::takeDmg(float a) {
    if (iframes > 0.f || victoryMode) return;
    hp = std::max(1.f, hp - a);
    iframes = 0.42f;
}

void SonicExe::update(float dt, MetalSonicController* player, DrRobotnik* robotnik, EventBus& ev,
                       const std::vector<Platform>& plats) {
    if (!active) return;
    p_ = player; r_ = robotnik; e_ = &ev;
    aurT += dt;
    if (iframes > 0.f) iframes -= dt;
    sm.tick(dt);
    if (!victoryMode) {
        x += vx * dt; y += vy * dt;
        resolveAABB(*this, plats);
        x = std::max(60.f, std::min(x, 1140.f));
    }
}

// ═══════════════════════════════════════════ DRAW ═══════════════════════════════════════════

void SonicExe::draw(sf::RenderTarget& rt, const Camera& cam) {
    if (!active && !victoryMode) return;
    float cx = cam.wx(x), by = cam.wy(y);
    float t = clk::now();
    float glitch = std::sin(t * 25.f) > 0.9f ? (gfx::frand() - 0.5f) * 7.f : 0.f;
    bool fr = faceR;

    float auraA = 0.1f + 0.08f * std::sin(aurT * 3.5f);
    gfx::fillCircle(rt, {cx, by - h * 0.5f}, w * 1.1f, gfx::rgba(255, 0, 0, auraA));

    gfx::px(rt, cx - 13, by - 8, 12, 7, gfx::hex("#1a0000"));
    gfx::px(rt, cx + 2, by - 8, 12, 7, gfx::hex("#1a0000"));
    gfx::px(rt, cx - 10, by - 22, 9, 14, gfx::hex("#0a0003"));
    gfx::px(rt, cx + 1, by - 22, 9, 14, gfx::hex("#0a0003"));
    gfx::px(rt, cx - 13 + glitch * 0.4f, by - 42, 26, 16, gfx::hex("#080000"));
    gfx::px(rt, cx - 10 + glitch * 0.3f, by - 40, 20, 8, gfx::hex("#0d0003"));
    float sD = fr ? -1.f : 1.f;
    for (int i = 0; i < 3; ++i) {
        gfx::px(rt, cx + sD * (10.f + (float)i * 4.f) - (7.f - (float)i) / 2.f,
                by - h + 6.f - (10.f + (float)i * 6.f) + 14.f, 7.f - (float)i, 10.f + (float)i * 6.f,
                gfx::hex("#0d0000"));
    }
    gfx::px(rt, cx - 14 + glitch, by - h + 6, 28, 22, gfx::hex("#0a0000"));
    gfx::px(rt, cx - 12 + glitch, by - h + 6, 24, 4, gfx::hex("#160000"));
    gfx::px(rt, cx - 14 + glitch, by - h + 6, 3, 3, gfx::hex("#000000"));
    gfx::px(rt, cx + 11 + glitch, by - h + 6, 3, 3, gfx::hex("#000000"));
    gfx::px(rt, cx - 14 + glitch, by - h + 25, 3, 3, gfx::hex("#000000"));
    gfx::px(rt, cx + 11 + glitch, by - h + 25, 3, 3, gfx::hex("#000000"));
    float eX = fr ? cx - 3 + glitch : cx - 11 + glitch;
    gfx::px(rt, eX, by - h + 14, 20, 7, gfx::hex("#ff0000"));
    gfx::px(rt, eX, by - h + 14, 20, 2, gfx::hex("#ff5555"));
    gfx::px(rt, eX - 3, by - h + 11, 26, 12, gfx::hexA("#ff0000", 0.48f * 0.4f));
    gfx::px(rt, eX + 2 + glitch, by - h + 21, 2, 16, gfx::hex("#660000"));
    gfx::px(rt, eX + 10 + glitch, by - h + 21, 2, 12, gfx::hex("#550000"));
    {
        sf::Vector2f p0{cx - 8 + glitch * 0.3f, by - h + 32};
        sf::Vector2f p1{cx - 3 + glitch * 0.3f, by - h + 37};
        sf::Vector2f p2{cx + glitch * 0.3f, by - h + 35};
        sf::Vector2f p3{cx + 5 + glitch * 0.3f, by - h + 38};
        sf::Vector2f p4{cx + 9 + glitch * 0.3f, by - h + 32};
        sf::Color grinCol = gfx::hex("#330000");
        gfx::thickLine(rt, p0, p1, 2.f, grinCol);
        gfx::thickLine(rt, p1, p2, 2.f, grinCol);
        gfx::thickLine(rt, p2, p3, 2.f, grinCol);
        gfx::thickLine(rt, p3, p4, 2.f, grinCol);
    }
    if (phase == 2) {
        float rp = 0.28f + 0.2f * std::sin(aurT * 6.f);
        for (int i = 1; i <= 3; ++i) {
            float radius = w * (0.65f + (float)i * 0.42f) + std::sin(aurT * 4.f + (float)i) * 5.f;
            gfx::strokeCircle(rt, {cx, by - h * 0.5f}, radius, 2.f, gfx::rgba(255, 0, 0, rp));
        }
    }
}
