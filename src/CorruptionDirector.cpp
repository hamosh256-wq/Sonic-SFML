#include "CorruptionDirector.hpp"
#include "Common.hpp"
#include <cmath>
#include <algorithm>

namespace {
// Entirely fictional in-universe system names - never your project's
// real files. Revealed one line at a time during ErrorLog/FileScramble.
const std::vector<std::string> kFakeLog = {
    "REALITY_ANCHOR.DLL ......... DESYNCED",
    "CURSE_CONTAINMENT.LOCK ..... BREACHED",
    "PHANTOM_RUBY_CORE .......... OVERLOAD",
    "G.U.N. FIREWALL ............ BYPASSED",
    "METAL_CORE.SYS ............. UNSTABLE",
    "EXE_SIGNATURE ............... SPREADING",
    "HOST PROCESS ................ NOT RESPONDING",
    "ATTEMPTING ROLLBACK ......... FAILED",
    "ATTEMPTING ROLLBACK ......... FAILED",
    "CONTAINMENT ................. LOST",
};

constexpr float T_FLICKER = 1.2f;
constexpr float T_ERRORLOG = 2.8f;
constexpr float T_SCRAMBLE = 2.6f;
constexpr float T_HARDCUT = 0.9f;
constexpr float T_AFTERMATH = 4.5f;
}

CorruptionDirector::CorruptionDirector(EventBus& ev, sf::Font& font, sf::Vector2u screenSize)
    : font_(font), screen_(screenSize) {
    ev.on("hyper_vs_exe", [this](const EventData&) { trigger(); });
}

void CorruptionDirector::trigger() {
    if (stage_ != Stage::Idle) return; // already running - don't restart mid-sequence
    enterStage(Stage::Flicker);
}

void CorruptionDirector::enterStage(Stage s) {
    stage_ = s;
    stageT_ = 0.f;
    if (s == Stage::ErrorLog) linesShown_ = 0;
}

void CorruptionDirector::update(float dt) {
    if (stage_ == Stage::Idle || stage_ == Stage::Done) return;
    stageT_ += dt;

    switch (stage_) {
        case Stage::Flicker:
            shake_ = std::min(1.f, stageT_ / T_FLICKER);
            if (stageT_ >= T_FLICKER) enterStage(Stage::ErrorLog);
            break;
        case Stage::ErrorLog: {
            std::size_t target = (std::size_t)((stageT_ / T_ERRORLOG) * (kFakeLog.size() * 0.6f));
            linesShown_ = std::min(target, kFakeLog.size());
            if (stageT_ >= T_ERRORLOG) enterStage(Stage::FileScramble);
            break;
        }
        case Stage::FileScramble: {
            std::size_t target = (std::size_t)((stageT_ / T_SCRAMBLE) * kFakeLog.size());
            linesShown_ = std::min(target, kFakeLog.size());
            shake_ = 1.f;
            if (stageT_ >= T_SCRAMBLE) enterStage(Stage::HardCut);
            break;
        }
        case Stage::HardCut:
            if (stageT_ >= T_HARDCUT) enterStage(Stage::Aftermath);
            break;
        case Stage::Aftermath:
            if (stageT_ >= T_AFTERMATH) enterStage(Stage::Done);
            break;
        default: break;
    }
}

void CorruptionDirector::draw(sf::RenderTarget& rt) const {
    if (stage_ == Stage::Idle || stage_ == Stage::Done) return;
    float W = (float)screen_.x, H = (float)screen_.y;

    if (stage_ == Stage::HardCut) {
        // One white flash on entry, then settle to black.
        float flash = std::max(0.f, 1.f - stageT_ * 6.f);
        gfx::px(rt, 0, 0, W, H, sf::Color::Black);
        if (flash > 0.f) gfx::px(rt, 0, 0, W, H, gfx::rgba(255, 255, 255, flash));
        return;
    }

    if (stage_ == Stage::Aftermath) {
        gfx::px(rt, 0, 0, W, H, sf::Color::Black);
        float fadeIn = std::min(1.f, stageT_ / 1.5f);
        gfx::retroText(rt, font_, "...containment holding.", W / 2.f, H / 2.f - 20.f, 16,
                       gfx::rgba(200, 220, 255, fadeIn), sf::Color::Black, gfx::Align::Center);
        if (stageT_ > 2.f) {
            gfx::retroText(rt, font_, "[R] RESTART", W / 2.f, H / 2.f + 24.f, 12,
                           gfx::rgba(140, 160, 200, std::min(1.f, (stageT_ - 2.f))), sf::Color::Black, gfx::Align::Center);
        }
        return;
    }

    // Flicker / ErrorLog / FileScramble share the same glitch backdrop,
    // escalating via `shake_`.
    sf::Color dim = gfx::rgba(0, 0, 0, 0.55f + 0.25f * shake_);
    gfx::px(rt, 0, 0, W, H, dim);

    // Horizontal tear bars, offset a few px each, cheap chromatic-
    // aberration stand-in (no shader pass wired up in this project - see
    // Common.hpp's radialGlow for the same tradeoff elsewhere).
    int tears = (int)(6 + shake_ * 18);
    for (int i = 0; i < tears; ++i) {
        float y = gfx::frand() * H;
        float h = 1.f + gfx::frand() * (3.f + shake_ * 6.f);
        float ox = (gfx::frand() - 0.5f) * 18.f * shake_;
        gfx::px(rt, ox, y, W, h, gfx::rgba(255, 0, 40, 0.10f + 0.1f * shake_));
        gfx::px(rt, -ox, y + 1.f, W, h, gfx::rgba(0, 220, 255, 0.08f + 0.08f * shake_));
    }

    // Fake system log, monospace, terminal-green-on-black.
    float logX = W * 0.5f - 190.f;
    float logY = H * 0.5f - 90.f + (gfx::frand() - 0.5f) * shake_ * 4.f;
    gfx::px(rt, logX - 14.f, logY - 14.f, 400.f, 20.f + (float)linesShown_ * 20.f, gfx::rgba(0, 0, 0, 0.7f));
    for (std::size_t i = 0; i < linesShown_; ++i) {
        sf::Color col = (i + 1 == linesShown_) ? sf::Color(255, 90, 90) : sf::Color(90, 220, 120);
        gfx::retroText(rt, font_, kFakeLog[i], logX, logY + (float)i * 20.f, 11, col, sf::Color::Black,
                       gfx::Align::Left, false, 1.f);
    }

    if (stage_ == Stage::Flicker && stageT_ < 0.3f) {
        gfx::retroText(rt, font_, "SIGNAL LOST", W / 2.f, H * 0.28f, 22, sf::Color(255, 40, 40),
                       sf::Color::Black, gfx::Align::Center);
    }
}
