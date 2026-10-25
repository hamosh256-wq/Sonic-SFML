#include "Game.hpp"
#include "Common.hpp"
#include "Clock.hpp"
#include <cmath>
#include <algorithm>

Game::Game(float W, float H, const std::string& fontPath, const sf::Texture& metalTexture)
    : W_(W), H_(H), cam_(W, H), ui_(W, H, fontPath),
      director_(ev_, ui_.font, {(unsigned)W, (unsigned)H}) {
    // Constructed exactly once, here. Every subsequent respawn/level-load
    // goes through metal_->reset(x, y) - see loadLevel() below - never
    // through reassignment or reconstruction (MetalSonicController holds
    // an EventBus& reference member, so it isn't copy/move-assignable by
    // design; trying to write `metal_ = std::make_unique<...>(...)` again
    // later would still compile fine here since metal_ is a unique_ptr
    // (only the pointer would be reassigned) - the actual constraint is
    // that a fresh MetalSonicController instance would need to be handed
    // this same ev_ reference again anyway, so there's no benefit to
    // reconstructing over resetting, and reset() is cheaper).
    metal_ = std::make_unique<MetalSonicController>(metalTexture, W_ / 2.f, H_ / 2.f, ev_);

    for (int i = 0; i < 100; ++i) {
        stars_.push_back(StarDef{
            gfx::frand() * 960.f, gfx::frand() * 600.f,
            gfx::frand() > 0.88f ? 2.f : 1.f,
            0.3f + gfx::frand() * 0.7f,
            0.04f + gfx::frand() * 0.18f,
            0.5f + gfx::frand() * 2.5f,
            gfx::frand() > 0.8f ? gfx::hex("#88ccff") : sf::Color::White
        });
    }
    bgGears_ = {
        BgGear{200.f, 490.f, 55.f, 10, 0.f, 0.08f},
        BgGear{540.f, 520.f, 40.f, 8, 1.5f, -0.12f},
        BgGear{840.f, 500.f, 65.f, 12, 0.8f, 0.06f},
    };
    wireEvents();
}

void Game::wireEvents() {
    // Phantom Ruby / Curse events - replace the old ability-HUD events
    // (shield_on/repair/drone/od_on), which don't exist on
    // MetalSonicController at all.
    ev_.on("phantom_ruby_activate", [this](const EventData& d) {
        ui_.notify("\u2726 PHANTOM RUBY ACTIVATING", 1.6f, gfx::hex("#ffc400"));
        ptcl_.burst(d.x, d.y - 30.f, 10, gfx::hex("#ffc400"), 100.f, 0.6f);
    });
    ev_.on("phantom_ruby_super", [this](const EventData& d) {
        ui_.notify("\u2605 SUPER METAL SONIC \u2605", 2.2f, gfx::hex("#66ccff"));
        cam_.shake(16.f, 0.5f);
        ptcl_.burst(d.x, d.y - 24.f, 22, gfx::hex("#66ccff"), 170.f, 0.9f);
    });
    ev_.on("phantom_ruby_hyper", [this](const EventData& d) {
        ui_.notify("\u2605 HYPER METAL SONIC \u2605", 2.6f, gfx::hex("#ffcc00"));
        cam_.shake(26.f, 1.0f);
        ptcl_.burst(d.x, d.y - 26.f, 34, gfx::hex("#ffcc00"), 230.f, 1.2f);
    });
    // Deliberately NOT also handling "hyper_vs_exe" here - that event is
    // CorruptionDirector's exclusively (it self-subscribes in its own
    // constructor above). Game doesn't need to know the sequence is
    // running at all; it just keeps calling director_.update()/draw()
    // every frame regardless, same as it would for any other overlay.
    ev_.on("curse_overwhelmed", [this](const EventData&) {
        ui_.notify("\u26a0 SYSTEMS OVERWHELMED", 1.6f, gfx::hex("#ff3333"));
        cam_.shake(10.f, 0.3f);
    });
    ev_.on("hard_landing", [this](const EventData& d) {
        cam_.shake(9.f, 0.22f);
        ptcl_.burst(d.x, d.y, 6, gfx::hex("#8899aa"), 80.f, 0.4f);
    });

    // Unchanged from before - these are SonicExe/DrRobotnik-driven, not
    // affected by the MetalSonic -> MetalSonicController swap.
    ev_.on("exe_teleport", [this](const EventData& d) {
        cam_.shake(14.f, 0.32f);
        ptcl_.burst(d.x, d.y - 22.f, 16, gfx::hex("#cc0000"), 170.f, 0.65f);
    });
    ev_.on("exe_hit_player", [this](const EventData&) {
        cam_.shake(20.f, 0.46f);
        if (metal_) {
            ptcl_.burst(metal_->posX(), metal_->posY() - 22.f, 12, gfx::hex("#ff4444"), 140.f, 0.75f);
            impactStars_.emplace_back(metal_->posX(), metal_->posY() - 22.f, gfx::hex("#ff4444"), false);
        }
    });
    ev_.on("exe_hit_robotnik", [this](const EventData&) {
        cam_.shake(24.f, 0.56f);
        ui_.notify("\u26a0 DR. ROBOTNIK UNDER ATTACK", 1.5f, gfx::hex("#ff6600"));
    });
    ev_.on("reinforce", [this](const EventData&) {
        if (phase_ == "boss1" || phase_ == "boss2") {
            allies_.emplace_back(180.f + gfx::frand() * 120.f, 579.f, RobotType::Ally);
            allies_.back().mode = "attack";
        }
    });
}

void Game::reset() {
    // metal_ is NOT nulled/reconstructed here - see the constructor
    // comment. Its state gets put back to spawn the moment goPhase1()
    // (via loadLevel()) runs again, through metal_->reset(x, y).
    exe_.reset(); robotnik_.reset(); tailsDoll_.reset();
    enemies_.clear(); allies_.clear(); rescueTgt_.reset(); level_ = nullptr;
    explosions_.clear(); impactStars_.clear(); hitStop_ = 0.f; ptcl_.pool.clear();
    phase_ = "title"; phaseName_.clear(); gameOver_ = false;
    transT_ = -1.f; transNext_.clear();
    endActive_ = false; endT_ = 0.f; endStep_ = 0; endFade_ = 0.f;
    score_ = 0; gameTime_ = 0.f;
    director_.resetToIdle();
}

void Game::loadLevel(const std::string& key) {
    const LevelData& ld = getLevel(key);
    level_ = &ld;
    metal_->reset(ld.spawnX, ld.spawnY); // always reset(), never reconstruct - see class comment
    metal_->setFacingCorruptionEntity(false);

    enemies_.clear();
    for (const auto& [ex, ey] : ld.enemies) enemies_.emplace_back(ex, ey, RobotType::Enemy);
    allies_.clear();
    rescueTgt_.reset();
    if (ld.rescue) rescueTgt_ = std::make_unique<RescueTarget>(ld.rescue->x, ld.rescue->y, ld.rescue->name);
    cam_.setX(std::max(0.f, ld.spawnX - W_ / 2.f));
    cam_.setY(0.f);
    transT_ = -1.f;
}

void Game::goPhase1() {
    loadLevel("phase1");
    phase_ = "play1";
    phaseName_ = getLevel("phase1").name;
    ui_.notify("\u2014 " + phaseName_ + " \u2014", 3.f, gfx::hex("#9999ff"));
}

void Game::goPhase2() {
    loadLevel("phase2");
    phase_ = "play2";
    phaseName_ = getLevel("phase2").name;
    ui_.notify("\u2014 " + phaseName_ + " \u2014", 3.f, gfx::hex("#9999ff"));
}

void Game::goPhase3() {
    loadLevel("phase3");
    phase_ = "cutscene";
    phaseName_ = getLevel("phase3").name;
    robotnik_ = std::make_unique<DrRobotnik>(920.f, 579.f);
    tailsDoll_ = std::make_unique<TailsDoll>();
    tailsDoll_->x = 1000.f; tailsDoll_->y = 508.f;
    tailsDoll_->activate(robotnik_.get());
    ui_.queueDlg({
        "Ivo Robotnik: \"Did you manage to save them, Metal?\"",
        "Metal Sonic: \"[Static... Processing...] Yes, Doctor.\"",
        "Ivo Robotnik: \"And have they joined our cause?\"",
        "Metal Sonic: \"The choice is not mine to make. But they are safe.\""
    });
    ui_.notify("\u2014 " + phaseName_ + " \u2014", 3.f, gfx::hex("#9999ff"));
}

void Game::goBoss1() {
    phase_ = "boss1";
    phaseName_ = "BOSS PHASE \u2160 \u2014 DEFEND THE DOCTOR";
    exe_ = std::make_unique<SonicExe>(720.f, 579.f);
    metal_->setFacingCorruptionEntity(true); // NEW: tells the Curse FSM Hyper is reachable
    allies_.clear();
    allies_.emplace_back(310.f, 579.f, RobotType::Ally); allies_.back().mode = "defend";
    allies_.emplace_back(420.f, 579.f, RobotType::Ally); allies_.back().mode = "defend";
    allies_.emplace_back(810.f, 579.f, RobotType::Ally); allies_.back().mode = "defend";
    ui_.notify("\u26a0 BASE BREACH \u2014 SONIC.EXE INCOMING \u26a0", 3.5f, gfx::hex("#ff1111"));
    cam_.shake(24.f, 1.2f);
    ptcl_.burst(720.f, 550.f, 32, gfx::hex("#cc0000"), 205.f, 1.0f);
}

void Game::goBoss2() {
    phase_ = "boss2";
    phaseName_ = "BOSS PHASE \u2161 \u2014 CHAOS UNLEASHED";
    if (exe_) exe_->setPhase(2);
    allies_.emplace_back(225.f, 579.f, RobotType::Ally); allies_.back().mode = "attack";
    allies_.emplace_back(960.f, 579.f, RobotType::Ally); allies_.back().mode = "attack";
    ui_.notify("\u2014 CURSE-FUELED HYPER FORM NOW REACHABLE \u2014", 3.5f, gfx::hex("#ffcc00"));
    cam_.shake(26.f, 1.4f);
}

void Game::triggerEnding() {
    if (endActive_) return;
    endActive_ = true; endT_ = 0.f; endStep_ = 0; endFade_ = 0.f; phase_ = "ending";
    if (metal_) metal_->takeDamage(metal_->maxRings()); // forces hp to 0 -> isActive() false
    if (exe_ && metal_) {
        exe_->victoryMode = true; exe_->sm.go("Victory");
        exe_->x = metal_->posX() + 25.f; exe_->y = metal_->posY() - 8.f;
    }
}

void Game::tickTrans(float dt) {
    if (transT_ < 0.f) return;
    transT_ -= dt;
    if (transT_ <= 0.f) {
        transT_ = -1.f;
        std::string n = transNext_; transNext_.clear();
        if (n == "play2") goPhase2();
        else if (n == "phase3") goPhase3();
        else if (n == "boss1") goBoss1();
    }
}

// ═══════════════════════════════════════════ UPDATE ═══════════════════════════════════════════

void Game::update(float dt) {
    if (inp.just(GKey::R) && (gameOver_ || (endActive_ && endFade_ > 0.92f) || director_.isFinished())) {
        reset();
        return;
    }

    // The corruption sequence takes over the whole screen once triggered -
    // tick it every frame regardless of phase (so it can pick up the
    // instant MetalSonicController's Hyper state fires "hyper_vs_exe"
    // mid-update below), then short-circuit everything else while active.
    director_.update(dt);
    if (director_.isActive()) {
        for (auto& e : explosions_) e.update(dt);
        explosions_.erase(std::remove_if(explosions_.begin(), explosions_.end(),
                           [](const Explosion& e) { return !e.active; }), explosions_.end());
        ptcl_.update(dt);
        return;
    }

    if (phase_ == "title") { if (inp.just(GKey::Enter)) goPhase1(); ui_.update(dt); return; }
    if (gameOver_) { ui_.update(dt); return; }

    if (hitStop_ > 0.f) {
        hitStop_ -= dt;
        for (auto& e : explosions_) e.update(dt);
        explosions_.erase(std::remove_if(explosions_.begin(), explosions_.end(),
                           [](const Explosion& e) { return !e.active; }), explosions_.end());
        for (auto& s : impactStars_) s.update(dt);
        impactStars_.erase(std::remove_if(impactStars_.begin(), impactStars_.end(),
                            [](const ImpactStar& s) { return !s.active; }), impactStars_.end());
        ui_.update(dt);
        return;
    }

    if (phase_ != "cutscene" && phase_ != "ending") gameTime_ += dt;

    if (phase_ == "cutscene") {
        if (inp.just(GKey::Space) && ui_.hasDlg()) {
            ui_.advDlg();
            if (!ui_.hasDlg()) { transT_ = 1.4f; transNext_ = "boss1"; }
        }
        if (tailsDoll_) tailsDoll_->update(dt);
        if (robotnik_) robotnik_->update(dt, ev_);
        tickTrans(dt);
        ui_.update(dt);
        return;
    }

    if (phase_ == "ending") {
        endT_ += dt;
        if (endT_ > 2.2f && endStep_ == 0) {
            endStep_ = 1; cam_.shake(22.f, 1.1f);
            ui_.notify("MECHA SONIC & SILVER METAL \u2014 LAST STAND ENGAGED", 3.5f, gfx::hex("#ff7700"));
        }
        if (endT_ > 5.8f && endStep_ == 1) {
            endStep_ = 2; cam_.shake(28.f, 1.6f);
            if (tailsDoll_) tailsDoll_->activateRuby();
            ui_.notify("TAILS DOLL \u2014 PHANTOM RUBY ACTIVATED", 3.f, gfx::hex("#ff00ff"));
        }
        if (endT_ > 9.0f) endFade_ = std::min(1.f, endFade_ + dt * 0.65f);
        if (tailsDoll_) tailsDoll_->update(dt);
        if (metal_ && level_) cam_.follow(metal_->posX(), metal_->posY(), level_->w, level_->h);
        for (auto& e : explosions_) e.update(dt);
        explosions_.erase(std::remove_if(explosions_.begin(), explosions_.end(),
                           [](const Explosion& e) { return !e.active; }), explosions_.end());
        for (auto& s : impactStars_) s.update(dt);
        impactStars_.erase(std::remove_if(impactStars_.begin(), impactStars_.end(),
                            [](const ImpactStar& s) { return !s.active; }), impactStars_.end());
        ptcl_.update(dt);
        ui_.update(dt);
        return;
    }

    if (!level_) return;
    const LevelData& lv = *level_;
    const auto& plats = lv.platforms;

    // Metal Sonic update - NEW 3-arg signature (EventBus is a stored
    // reference from construction, not passed per-call; level bounds
    // aren't tracked internally, so Game clamps them right here instead).
    if (metal_) {
        metal_->update(dt, inp, plats);
        metal_->body().x = std::clamp(metal_->body().x, 20.f, lv.w - 20.f);

        if ((metal_->formState() == "Super" || metal_->formState() == "Hyper") && gfx::frand() < 0.4f) {
            float dir = metal_->facingRight() ? -1.f : 1.f;
            ptcl_.spark(metal_->posX() + dir * 10.f, metal_->posY() - 20.f, dir,
                        metal_->formState() == "Hyper" ? gfx::hex("#ffcc00") : gfx::hex("#66ccff"), 3);
        }
        if (!metal_->isActive() && !endActive_) {
            if (phase_ == "boss1" || phase_ == "boss2") triggerEnding();
            else gameOver_ = true;
        }
    }

    // Enemies
    for (auto& e : enemies_) {
        e.update(dt, plats, metal_.get());
        if (e.active && metal_ && metal_->isAttacking()) {
            if (std::hypot(metal_->posX() - e.x, metal_->posY() - 16.f - e.y) < 52.f) {
                e.takeDmg(metal_->atk());
                if (!e.active) {
                    explosions_.emplace_back(e.x, e.y - 16.f, 1.0f);
                    impactStars_.emplace_back(e.x, e.y - 16.f, gfx::hex("#ffcc00"), false);
                    cam_.shake(14.f, 0.36f);
                    hitStop_ = std::max(hitStop_, 0.05f);
                    score_ += 100;
                } else {
                    impactStars_.emplace_back(e.x, e.y - 16.f, gfx::hex("#ffaa00"), false);
                    hitStop_ = std::max(hitStop_, 0.03f);
                }
            }
        }
    }
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                   [](const Robot& r) { return !r.active; }), enemies_.end());

    // Allies
    for (auto& a : allies_) a.update(dt, plats, exe_.get());
    allies_.erase(std::remove_if(allies_.begin(), allies_.end(),
                  [](const Robot& r) { return !r.active; }), allies_.end());

    // Rescue target
    if (rescueTgt_) {
        rescueTgt_->update(dt);
        if (rescueTgt_->active && metal_ &&
            std::hypot(metal_->posX() - rescueTgt_->x, metal_->posY() - rescueTgt_->y) < 68.f) {
            rescueTgt_->active = false; rescueTgt_->rescued = true;
            std::string nm = rescueTgt_->name;
            std::transform(nm.begin(), nm.end(), nm.begin(), [](unsigned char c) { return std::toupper(c); });
            ui_.notify("\u2713 " + nm + " RESCUED \u2014 RETURNING TO BASE", 3.2f, gfx::hex("#00e676"));
            cam_.shake(14.f, 0.46f);
            ptcl_.burst(rescueTgt_->x, rescueTgt_->y - 20.f, 26, gfx::hex("#00e676"), 160.f, 1.1f);
            score_ += 500;
            transT_ = 3.2f;
            transNext_ = (phase_ == "play1") ? "play2" : "phase3";
        }
    }

    // Sonic.EXE boss
    if (exe_ && !endActive_) {
        exe_->update(dt, metal_.get(), robotnik_.get(), ev_, plats);
        if (metal_ && metal_->isAttacking() && !exe_->victoryMode && exe_->iframes <= 0.f) {
            if (std::hypot(metal_->posX() - exe_->x, metal_->posY() - 24.f - exe_->y) < 62.f) {
                exe_->takeDmg(metal_->atk());
                score_ += 50;
                bool empowered = (metal_->formState() == "Super" || metal_->formState() == "Hyper");
                impactStars_.emplace_back(exe_->x, exe_->y - 24.f,
                                          empowered ? gfx::hex("#00ffff") : gfx::hex("#ff4400"), true);
                cam_.shake(18.f, 0.38f);
                hitStop_ = std::max(hitStop_, 0.06f);
                ptcl_.burst(exe_->x, exe_->y - 24.f, 14, gfx::hex("#ff0000"), 155.f, 0.8f);
                if (exe_->hp < exe_->maxHp * 0.11f) triggerEnding();
            }
        }
        if (phase_ == "boss1") {
            bool lowM = metal_ && metal_->rings() < metal_->maxRings() * 0.28f;
            bool lowR = robotnik_ && robotnik_->hp < robotnik_->maxHp * 0.5f;
            if (exe_->hp <= exe_->maxHp * 0.5f || lowM || lowR) goBoss2();
        }
        if (robotnik_ && robotnik_->hp <= 0.f && !endActive_) gameOver_ = true;
    }

    if (robotnik_) robotnik_->update(dt, ev_);
    if (tailsDoll_) tailsDoll_->update(dt);
    tickTrans(dt);

    if (metal_ && level_) cam_.follow(metal_->posX(), metal_->posY(), lv.w, lv.h);

    for (auto& e : explosions_) e.update(dt);
    explosions_.erase(std::remove_if(explosions_.begin(), explosions_.end(),
                       [](const Explosion& e) { return !e.active; }), explosions_.end());
    for (auto& s : impactStars_) s.update(dt);
    impactStars_.erase(std::remove_if(impactStars_.begin(), impactStars_.end(),
                        [](const ImpactStar& s) { return !s.active; }), impactStars_.end());
    ptcl_.update(dt);
    ui_.update(dt);

    for (auto& g : bgGears_) g.rot += g.spd * dt;
}

// ═══════════════════════════════════════════ DRAW ═══════════════════════════════════════════

void Game::draw(sf::RenderTarget& rt) {
    if (phase_ == "title") { ui_.drawTitle(rt); return; }
    if (gameOver_) { ui_.drawGameOver(rt); return; }

    if (!level_) {
        // Still draw something sensible if the corruption sequence somehow
        // fires before any level ever loaded (shouldn't happen in normal
        // play, but director_ owns the whole screen in that case anyway).
        director_.draw(rt);
        return;
    }

    drawBg(rt);
    drawPlats(rt);
    if (!level_->distortion.empty()) drawDistortion(rt);

    if (rescueTgt_) rescueTgt_->draw(rt, cam_, ui_.font);
    for (auto& e : enemies_) e.draw(rt, cam_);
    for (auto& a : allies_) a.draw(rt, cam_);
    if (robotnik_) robotnik_->draw(rt, cam_);
    if (tailsDoll_) tailsDoll_->draw(rt, cam_);
    if (exe_) exe_->draw(rt, cam_);

    // MetalSonicController is camera-agnostic by design (see
    // MetalSonicController.hpp) - it keeps its sprite in world-space
    // internally so the standalone demo works with no camera at all.
    // Game overrides that to screen-space right before drawing, the same
    // way every other entity here is positioned via cam_.wx()/wy().
    if (metal_) {
        metal_->setRenderPosition({cam_.wx(metal_->posX()), cam_.wy(metal_->posY())});
        metal_->draw(rt);
    }

    for (auto& expl : explosions_) expl.draw(rt, cam_);
    for (auto& star : impactStars_) star.draw(rt, cam_);
    ptcl_.draw(rt, cam_);

    if (phase_ == "ending" && endStep_ >= 1) drawEndingHeroes(rt);

    if (endFade_ > 0.f) gfx::px(rt, 0, 0, W_, H_, gfx::rgba(0, 0, 0, endFade_));
    if (phase_ == "ending") ui_.drawEnd(rt, endFade_);

    bool showEXE = (phase_ == "boss1" || phase_ == "boss2" || phase_ == "ending") && exe_;
    bool isBoss = (phase_ == "boss1" || phase_ == "boss2");
    ui_.draw(rt, metal_.get(), showEXE ? exe_.get() : nullptr,
             isBoss ? robotnik_.get() : nullptr, isBoss, phaseName_, score_, gameTime_);

    // Drawn last, on top of literally everything else - this is the
    // "self-destruct" overlay taking over the screen once triggered.
    director_.draw(rt);
}

void Game::drawBg(sf::RenderTarget& rt) {
    float t = clk::now();
    gfx::px(rt, 0, 0, W_, H_, level_ && !level_->bg.empty() ? gfx::hex(level_->bg) : gfx::hex("#06060e"));

    for (const auto& s : stars_) {
        float sx = std::fmod(std::fmod(s.x - cam_.x() * s.spd + t * s.spd * 8.f, W_) + W_, W_);
        float sy = std::fmod(std::fmod(s.y - cam_.y() * s.spd * 0.5f, H_) + H_, H_);
        float a = s.bright * (0.55f + 0.45f * std::sin(t * s.twinkle));
        sf::Color c = s.col; c.a = (std::uint8_t)(std::clamp(a, 0.f, 1.f) * 255.f);
        gfx::px(rt, sx, sy, s.sz, s.sz, c);
    }

    float gx0 = std::fmod(cam_.x() * 0.12f, 60.f), gy0 = std::fmod(cam_.y() * 0.12f, 60.f);
    sf::Color gridCol = gfx::hex("#00ffff07");
    for (float x = -gx0; x < W_ + 60.f; x += 60.f) gfx::thickLine(rt, {x, 0.f}, {x, H_}, 1.f, gridCol);
    for (float y = -gy0; y < H_ + 60.f; y += 60.f) gfx::thickLine(rt, {0.f, y}, {W_, y}, 1.f, gridCol);

    for (const auto& g : bgGears_) {
        float sx = g.bx - cam_.x() * 0.28f;
        float sy = g.by - cam_.y() * 0.1f;
        if (sx > -g.r * 2.f && sx < W_ + g.r * 2.f) drawGear(rt, sx, sy, g.r, g.teeth, g.rot);
    }

    for (int i = -1; i < 8; ++i) {
        float period = 240.f * 8.f;
        float bx = std::fmod(std::fmod((float)i * 240.f - cam_.x() * 0.3f, period) + period, period) - 240.f;
        if (bx < -60.f || bx > W_ + 60.f) continue;
        gfx::px(rt, bx - 15.f, H_ * 0.55f, 30.f, H_ * 0.5f, gfx::hex("#0a0a16"));
        gfx::px(rt, bx - 6.f, H_ * 0.44f, 12.f, H_ * 0.56f, gfx::hex("#0a0a16"));
        if (std::sin(t * 1.5f + (float)i) > 0.f) {
            gfx::fillCircle(rt, {bx, H_ * 0.44f - 5.f}, 4.f, gfx::hexA("#ff0000", 0x44 / 255.f));
        }
    }

    for (float y = 0; y < H_; y += 3.f) gfx::px(rt, 0, y, W_, 1, gfx::rgba(0, 0, 0, 0.14f));

    if (std::sin(t * 11.f) > 0.92f) {
        gfx::px(rt, 0, gfx::frand() * H_, W_, gfx::frand() * 4.f + 1.f, gfx::rgba(255, 0, 0, 0.04f));
    }
}

void Game::drawGear(sf::RenderTarget& rt, float gx, float gy, float r, int teeth, float rot) {
    sf::ConvexShape gear((std::size_t)(teeth * 2));
    for (int i = 0; i < teeth * 2; ++i) {
        float a = ((float)i / (float)(teeth * 2)) * 3.14159265f * 2.f + rot;
        float radius = (i % 2 == 0) ? r : r * 0.72f;
        gear.setPoint((std::size_t)i, {gx + std::cos(a) * radius, gy + std::sin(a) * radius});
    }
    gear.setFillColor(gfx::hex("#0d0d18"));
    rt.draw(gear);
    gfx::fillCircle(rt, {gx, gy}, r * 0.38f, gfx::hex("#06060e"));
    gfx::strokeCircle(rt, {gx, gy}, r * 0.6f, 1.f, gfx::hex("#00ffff08"));
}

void Game::drawPlats(sf::RenderTarget& rt) {
    for (const auto& p : level_->platforms) {
        float sx = cam_.wx(p.x), sy = cam_.wy(p.y);
        if (sx + p.w < 0.f || sx > W_ || sy + p.h < 0.f || sy > H_) continue;

        gfx::px(rt, sx, sy, p.w, p.h, gfx::hex("#16161f"));
        gfx::px(rt, sx, sy, p.w, std::min(5.f, p.h), gfx::hex("#1e1e2a"));

        sf::Color gridCol = gfx::hex("#00ffff0d");
        for (float cy = 8.f; cy < p.h; cy += 8.f) gfx::thickLine(rt, {sx, sy + cy}, {sx + p.w, sy + cy}, 1.f, gridCol);
        for (float cx = 10.f; cx < p.w; cx += 14.f) gfx::thickLine(rt, {sx + cx, sy}, {sx + cx, sy + p.h}, 1.f, gridCol);

        for (float ni = 14.f; ni < p.w; ni += 28.f)
            for (float nj = 8.f; nj < p.h; nj += 16.f)
                if (std::fmod(ni + nj, 32.f) == 0.f) gfx::fillCircle(rt, {sx + ni, sy + nj}, 1.8f, gfx::hexA("#00ffff", 0x2a / 255.f));

        gfx::px(rt, sx, sy, p.w, 2, gfx::hexA("#00ffff", 0x55 / 255.f));

        if (p.h >= 30.f) {
            sf::View old = gfx::pushClip(rt, sx, sy, p.w, p.h);
            for (int i = -2; (float)i * 44.f < p.w + p.h + 88.f; ++i) {
                if (i % 2 == 0) {
                    sf::ConvexShape stripe(4);
                    stripe.setPoint(0, {sx + (float)i * 44.f, sy + p.h});
                    stripe.setPoint(1, {sx + (float)i * 44.f + p.h, sy});
                    stripe.setPoint(2, {sx + (float)(i + 1) * 44.f + p.h, sy});
                    stripe.setPoint(3, {sx + (float)(i + 1) * 44.f, sy + p.h});
                    stripe.setFillColor(gfx::hexA("#ffcc00", 0x10 / 255.f));
                    rt.draw(stripe);
                }
            }
            gfx::popClip(rt, old);
        }

        gfx::px(rt, sx, sy, 2, p.h, gfx::hexA("#000000", 0x44 / 255.f));
        gfx::px(rt, sx + p.w - 2, sy, 2, p.h, gfx::hexA("#ffffff", 0x07 / 255.f));
        gfx::px(rt, sx, sy + p.h - 2, p.w, 2, gfx::hexA("#000000", 0x33 / 255.f));
    }
}

void Game::drawDistortion(sf::RenderTarget& rt) {
    float t = clk::now();
    for (const auto& z : level_->distortion) {
        float cx = cam_.wx(z.x), cy = cam_.wy(z.y);
        float r = z.r * (1.f + 0.13f * std::sin(t * 3.2f));
        gfx::radialGlow(rt, {cx, cy}, r, gfx::rgba(255, 0, 0, 0.16f), gfx::rgba(255, 0, 0, 0.f), 6);
        sf::Color streakCol = gfx::rgba(255, 60, 60, 0.14f);
        for (int i = -2; i <= 2; ++i) {
            gfx::thickLine(rt, {cx + (float)i * 18.f, cy - r}, {cx + (float)i * 18.f, cy + r}, 1.f, streakCol);
        }
    }
}

void Game::drawEndingHeroes(sf::RenderTarget& rt) {
    float t = clk::now();

    float mx = cam_.wx(240.f), my = cam_.wy(578.f);
    gfx::px(rt, mx - 14, my - 42, 28, 34, gfx::hex("#555555"));
    gfx::px(rt, mx - 12, my - 42, 24, 4, gfx::hex("#777777"));
    gfx::fillCircle(rt, {mx, my - 52.f}, 13.f, gfx::hex("#777777"));
    gfx::px(rt, mx + 3, my - 56, 9, 5, gfx::hex("#ff6600"));
    gfx::retroText(rt, ui_.font, "MECHA SONIC", mx, my - 70, 8, gfx::hex("#ff8800"), sf::Color(0, 0, 0, 136), gfx::Align::Center);

    float sx = cam_.wx(960.f), sy = cam_.wy(578.f);
    gfx::px(rt, sx - 14, sy - 42, 28, 34, gfx::hex("#9999bb"));
    gfx::px(rt, sx - 12, sy - 42, 24, 4, gfx::hex("#bbbbdd"));
    gfx::fillCircle(rt, {sx, sy - 52.f}, 13.f, gfx::hex("#bbbbdd"));
    gfx::px(rt, sx + 3, sy - 56, 9, 5, gfx::hex("#aaaaff"));
    gfx::retroText(rt, ui_.font, "SILVER METAL", sx, sy - 70, 8, gfx::hex("#aaaaff"), sf::Color(0, 0, 0, 136), gfx::Align::Center);

    if (exe_) {
        float exCx = cam_.wx(exe_->x), exCy = cam_.wy(exe_->y) - 25.f;
        for (int i = 0; i < 7; ++i) {
            float a = ((float)i / 7.f) * 3.14159265f * 2.f + t * 3.f;
            float r = 60.f + (float)i * 16.f + std::sin(t * 6.f + (float)i) * 12.f;
            sf::Color col = gfx::rgba(255, 100 + i * 20, 0, 0.18f + 0.12f * std::sin(t * 14.f));
            gfx::thickLine(rt, {exCx, exCy}, {exCx + std::cos(a) * r, exCy + std::sin(a) * r}, 2.f, col);
        }
    }
}
