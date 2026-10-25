#include "UIManager.hpp"
#include "Common.hpp"
#include "Clock.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>

UIManager::UIManager(float w, float h, const std::string& fontPath) : W(w), H(h) {
    if (!font.openFromFile(fontPath)) {
        // Non-fatal: text just won't render if the font path is wrong.
    }
}

void UIManager::notify(const std::string& text, float dur, sf::Color col) {
    notifs_.push_back(Notif{text, dur, dur, col, H * 0.3f});
}

void UIManager::queueDlg(const std::vector<std::string>& lines) {
    dlgQueue_.assign(lines.begin(), lines.end());
    nextDlg();
}

void UIManager::nextDlg() {
    if (dlgQueue_.empty()) { curDlg_.reset(); return; }
    curDlg_ = dlgQueue_.front();
    dlgQueue_.pop_front();
    dlgT_ = 0.f;
}

void UIManager::advDlg() {
    if (curDlg_) { curDlg_.reset(); nextDlg(); }
}

void UIManager::flashKey(const std::string& k) { flash_[k] = 0.6f; }

void UIManager::update(float dt) {
    for (auto& n : notifs_) { n.t -= dt; n.oy -= 11.f * dt; }
    notifs_.erase(std::remove_if(notifs_.begin(), notifs_.end(),
                                  [](const Notif& n) { return n.t <= 0.f; }),
                  notifs_.end());
    if (curDlg_) dlgT_ += dt;
    for (auto it = flash_.begin(); it != flash_.end();) {
        it->second -= dt;
        if (it->second <= 0.f) it = flash_.erase(it); else ++it;
    }
}

// ═══════════════════════════════════════════ MAIN HUD ═══════════════════════════════════════════

void UIManager::draw(sf::RenderTarget& rt, MetalSonicController* metal, SonicExe* exe, DrRobotnik* robotnik,
                      bool isBoss, const std::string& phaseName, int score, float gameTime) {
    gfx::px(rt, 0, 0, W, 46, gfx::hex("#000000bb"));
    gfx::px(rt, 0, 45, W, 2, gfx::hex("#00ffff66"));

    gfx::retroText(rt, font, "SCORE", 16, 13, 10, gfx::hex("#00ffff"));
    gfx::retroText(rt, font, gfx::padStart(std::max(0, score), 8), 16, 29, 13, sf::Color::White);

    int mins = (int)std::floor(std::max(0.f, gameTime) / 60.f);
    int secs = (int)std::floor(std::fmod(std::max(0.f, gameTime), 60.f));
    gfx::retroText(rt, font, "TIME", W / 2.f - 38.f, 13, 10, gfx::hex("#00ffff"));
    gfx::retroText(rt, font, gfx::padStart(mins, 2) + "'" + gfx::padStart(secs, 2) + "\"",
                   W / 2.f - 38.f, 29, 13, sf::Color::White);

    // RINGS (was metal->hp on the old procedural class; MetalSonicController
    // exposes the same concept through rings()).
    int rings = metal ? std::max(0, (int)std::ceil(metal->rings())) : 0;
    sf::Color rc = rings > 60 ? gfx::hex("#00ffaa") : rings > 25 ? gfx::hex("#ffcc00") : gfx::hex("#ff4444");
    gfx::retroText(rt, font, "RINGS", W / 2.f + 40.f, 13, 10, gfx::hex("#00ffff"));
    gfx::retroText(rt, font, std::to_string(rings), W / 2.f + 40.f, 29, 13, rc);

    gfx::retroText(rt, font, phaseName, W - 16.f, 16, 9, gfx::hex("#8888cc"), gfx::hex("#000022"), gfx::Align::Right);

    if (exe && (exe->active || exe->victoryMode)) drawBossBar(rt, exe->hp, exe->maxHp);
    if (isBoss && robotnik) drawDocBar(rt, robotnik->hp, robotnik->maxHp);

    if (metal) drawFormHUD(rt, metal);

    for (const auto& n : notifs_) {
        float a = std::min(1.f, n.t * 2.5f);
        sf::Color shadow(0, 0, 0, (std::uint8_t)(a * 255.f));
        sf::Color col = gfx::mulAlpha(n.col, a);
        gfx::retroText(rt, font, n.text, W / 2.f, n.oy, 14, col, shadow, gfx::Align::Center);
    }

    if (curDlg_) drawDialog(rt, *curDlg_);
}

void UIManager::drawBossBar(sf::RenderTarget& rt, float hp, float maxHp) {
    float bx = W / 2.f - 192.f, by = H - 46.f, bw = 384.f, bh = 20.f;
    gfx::retroText(rt, font, "\u2726  SONIC.EXE  \u2726", W / 2.f, by - 8.f, 11,
                   gfx::hex("#ff3333"), sf::Color::Black, gfx::Align::Center, true, 1.f);

    gfx::px(rt, bx, by, bw, bh, gfx::hex("#200000"));
    int segs = 24;
    float sw = (bw - 2.f) / (float)segs;
    float pct = std::max(0.f, hp / maxHp);
    for (int i = 0; i < segs; ++i) {
        float frac = (float)i / (float)segs;
        if (frac < pct) {
            sf::Color seg = gfx::hsl(5.f - frac * 5.f, 1.f, (25.f + frac * 10.f) / 100.f);
            gfx::px(rt, bx + 1 + (float)i * sw + 1, by + 3, sw - 2, bh - 6, seg);
        }
    }
    gfx::strokeRect(rt, bx, by, bw, bh, 2.f, gfx::hex("#ff222266"));
    gfx::px(rt, bx + 1, by + 1, bw - 2, 2, gfx::hex("#ffffff18"));
}

void UIManager::drawDocBar(sf::RenderTarget& rt, float hp, float maxHp) {
    float bx = 12.f, by = H - 76.f, bw = 150.f, bh = 14.f;
    gfx::retroText(rt, font, "DR. IVO", bx, by - 5.f, 9, gfx::hex("#cc88ff"));
    gfx::px(rt, bx, by, bw, bh, gfx::hex("#180028"));
    float pw = (bw - 2.f) * std::max(0.f, hp / maxHp);
    gfx::px(rt, bx + 1, by + 1, pw, bh - 2, gfx::hex("#9944cc"));
    gfx::px(rt, bx + 1, by + 1, pw, 3, gfx::hex("#cc66ff"));
    gfx::strokeRect(rt, bx, by, bw, bh, 1.f, gfx::hex("#8844aa88"));
}

// ═══════════════════════════════════════════ FORM / CURSE HUD (new) ═══════════════════════════════════════════
// Replaces the old drawAbilityHUD's Q/E/R/F icon grid, which showed
// cooldowns for abilities MetalSonicController doesn't have. This shows
// what it actually tracks: current form state and the curse meter that
// drives Phantom Ruby -> Super -> Hyper.

void UIManager::drawFormHUD(sf::RenderTarget& rt, MetalSonicController* m) {
    float bx = 12.f, by = H - 68.f, bw = 230.f, bh = 46.f;

    const std::string& form = m->formState();
    sf::Color formCol = gfx::hex("#8888cc");
    std::string label = "NORMAL";
    if (form == "Charging")     { formCol = gfx::hex("#ffc400"); label = "PHANTOM RUBY CHARGING"; }
    else if (form == "Super")   { formCol = gfx::hex("#66ccff"); label = "SUPER METAL SONIC"; }
    else if (form == "Hyper")   { formCol = gfx::hex("#ffcc00"); label = "HYPER METAL SONIC"; }
    bool flash = (int)(clk::now() * 6.f) % 2 == 0;
    if (m->isGlitched()) { formCol = gfx::hex("#ff3333"); label = "SYSTEMS OVERWHELMED"; }

    gfx::px(rt, bx, by, bw, bh, gfx::hex("#000011cc"));
    gfx::strokeRect(rt, bx, by, bw, bh, 2.f, gfx::mulAlpha(formCol, m->isGlitched() && !flash ? 0.25f : 0.8f));
    gfx::retroText(rt, font, label, bx + 8, by + 6, 10, formCol);

    float pct = m->curse01();
    sf::Color curseCol = pct > 0.75f ? gfx::hex("#ff3333") : pct > 0.4f ? gfx::hex("#ffcc00") : gfx::hex("#66ff99");
    gfx::px(rt, bx + 8, by + 24, bw - 16, 10, gfx::hex("#180000"));
    gfx::px(rt, bx + 9, by + 25, (bw - 18) * pct, 8, curseCol);
    gfx::strokeRect(rt, bx + 8, by + 24, bw - 16, 10, 1.f, gfx::hexA("#ffffff", 0.25f));
    gfx::retroText(rt, font, "CURSE", bx + 8, by + 35, 7, gfx::hex("#8888aa"));

    gfx::retroText(rt, font, "ARROWS/WASD: MOVE  \u00b7  SPACE: JUMP  \u00b7  HOLD F: PHANTOM RUBY",
                   12, H - 8, 8, gfx::hex("#1a1a3344"), sf::Color::Transparent);
}

// ═══════════════════════════════════════════ DIALOG ═══════════════════════════════════════════

void UIManager::drawDialog(sf::RenderTarget& rt, const std::string& text) {
    float bx = W * 0.07f, by = H - 218.f, bw = W * 0.86f, bh = 128.f;
    gfx::px(rt, bx, by, bw, bh, gfx::hex("#000014ee"));
    gfx::px(rt, bx, by, bw, 3, gfx::hex("#3344aa")); gfx::px(rt, bx, by + bh - 3, bw, 3, gfx::hex("#3344aa"));
    gfx::px(rt, bx, by, 3, bh, gfx::hex("#3344aa")); gfx::px(rt, bx + bw - 3, by, 3, bh, gfx::hex("#3344aa"));
    gfx::px(rt, bx + 3, by + 3, bw - 6, 2, gfx::hex("#4455cc33"));

    sf::Color textCol = gfx::hex("#dde6ff");
    std::string line;
    float ly = by + 33.f;
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) {
        std::string test = line + word + " ";
        if (gfx::textWidth(font, test, 13) > bw - 32.f && !line.empty()) {
            std::string trimmed = line;
            while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();
            gfx::retroText(rt, font, trimmed, bx + 16, ly + 1, 13, textCol, gfx::hex("#000055"),
                           gfx::Align::Left, false, 1.f);
            line = word + " "; ly += 23.f;
        } else line = test;
    }
    {
        std::string trimmed = line;
        while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();
        gfx::retroText(rt, font, trimmed, bx + 16, ly + 1, 13, textCol, gfx::hex("#000055"),
                       gfx::Align::Left, false, 1.f);
    }

    if (dlgT_ > 0.7f && (clk::nowMs() / 500) % 2) {
        gfx::retroText(rt, font, "[SPACE] continue", bx + bw - 12, by + bh - 12, 10,
                       gfx::hexA("#ffffff", 0.33f), gfx::hex("#000033"), gfx::Align::Right, false, 1.f);
    }
}

// ═══════════════════════════════════════════ TITLE / GAME OVER / END ═══════════════════════════════════════════

void UIManager::drawTitle(sf::RenderTarget& rt) {
    float t = clk::now();
    gfx::px(rt, 0, 0, W, H, gfx::hex("#000008"));

    for (int i = 0; i < 100; ++i) {
        float sx = std::fmod(std::fmod((float)i * 137.f + t * 8.f, W) + W, W);
        float sy = std::fmod((float)(i * 97), H);
        float a = (0.35f + 0.4f * std::sin(t * 2.f + (float)i)) * 0.8f;
        sf::Color c = (i % 5 == 0) ? gfx::hex("#88ccff") : sf::Color::White;
        c.a = (std::uint8_t)(std::clamp(a, 0.f, 1.f) * 255.f);
        float sz = (i % 8 == 0) ? 2.f : 1.f;
        gfx::px(rt, sx, sy, sz, sz, c);
    }

    for (float y = 0; y < H; y += 3.f) gfx::px(rt, 0, y, W, 1, gfx::rgba(0, 0, 0, 0.18f));

    float g = std::sin(t * 18.f) > 0.88f ? (gfx::frand() - 0.5f) * 10.f : 0.f;
    gfx::plainText(rt, font, "SONIC.EXE", W / 2.f + g + 4.f, H / 2.f - 118.f, 58, gfx::hex("#220000"), gfx::Align::Center);
    gfx::plainText(rt, font, "SONIC.EXE", W / 2.f + g + 2.f, H / 2.f - 116.f, 58, gfx::hex("#440000"), gfx::Align::Center);
    gfx::plainText(rt, font, "SONIC.EXE", W / 2.f, H / 2.f - 118.f, 58, gfx::hex("#ff1111"), gfx::Align::Center);

    gfx::plainText(rt, font, "LAST PROTOCOL", W / 2.f + 2.f, H / 2.f - 72.f, 27, gfx::hex("#000044"), gfx::Align::Center);
    gfx::plainText(rt, font, "LAST PROTOCOL", W / 2.f, H / 2.f - 74.f, 27, gfx::hex("#8888ff"), gfx::Align::Center);

    gfx::plainText(rt, font, "\u25b8  A FAN-GAME PROTOTYPE  \u25c2", W / 2.f + 1.f, H / 2.f - 44.f, 11, gfx::hex("#334455"), gfx::Align::Center);
    gfx::plainText(rt, font, "\u25b8  A FAN-GAME PROTOTYPE  \u25c2", W / 2.f, H / 2.f - 46.f, 11, gfx::hex("#556677"), gfx::Align::Center);

    float bx = W / 2.f - 325.f, by = H / 2.f - 18.f, bw = 650.f, bh = 126.f;
    gfx::px(rt, bx, by, bw, bh, gfx::rgba(8, 8, 28, 0.96f));
    gfx::px(rt, bx, by, bw, 3, gfx::hex("#222266")); gfx::px(rt, bx, by + bh - 3, bw, 3, gfx::hex("#222266"));
    gfx::px(rt, bx, by, 3, bh, gfx::hex("#222266")); gfx::px(rt, bx + bw - 3, by, 3, bh, gfx::hex("#222266"));
    gfx::px(rt, bx + 3, by + 3, bw - 6, 2, gfx::hex("#3344aa22"));

    gfx::retroText(rt, font, "AS REALITY FRACTURES UNDER SONIC.EXE'S CORRUPTION,", W / 2.f, by + 26.f, 12,
                   gfx::hex("#7777aa"), gfx::hex("#00001188"), gfx::Align::Center);
    gfx::retroText(rt, font, "Dr. Ivo Robotnik commands the last resistance from the shadows.", W / 2.f, by + 50.f, 11,
                   gfx::hex("#555588"), gfx::hex("#00001188"), gfx::Align::Center);
    gfx::retroText(rt, font, "METAL SONIC \u2014 rebuilt, weaponized, reborn \u2014 is their only blade.", W / 2.f, by + 68.f, 11,
                   gfx::hex("#555588"), gfx::hex("#00001188"), gfx::Align::Center);

    bool blink = std::sin(t * 4.5f) > 0.f;
    gfx::retroText(rt, font, "\u25b6  PRESS  [ENTER]  TO BEGIN  \u25c0", W / 2.f, by + 100.f, 14,
                   blink ? gfx::hex("#00e5ff") : gfx::hex("#0099bb"), gfx::hex("#00002288"), gfx::Align::Center);
    gfx::retroText(rt, font, "ARROWS/WASD:MOVE \u00b7 SPACE:JUMP \u00b7 HOLD F:PHANTOM RUBY",
                   W / 2.f, H - 14.f, 8, gfx::hex("#1a1a3355"), sf::Color::Transparent, gfx::Align::Center);
}

void UIManager::drawGameOver(sf::RenderTarget& rt) {
    gfx::px(rt, 0, 0, W, H, gfx::rgba(0, 0, 0, 0.94f));
    gfx::plainText(rt, font, "GAME OVER", W / 2.f + 3.f, H / 2.f - 40.f, 52, gfx::hex("#330000"), gfx::Align::Center);
    gfx::plainText(rt, font, "GAME OVER", W / 2.f, H / 2.f - 42.f, 52, gfx::hex("#ff1111"), gfx::Align::Center);
    gfx::retroText(rt, font, "The resistance has fallen.", W / 2.f, H / 2.f + 14.f, 16,
                   gfx::hex("#882222"), gfx::hex("#000022"), gfx::Align::Center);
    gfx::retroText(rt, font, "[R] TO RESTART", W / 2.f, H / 2.f + 60.f, 13,
                   gfx::hex("#334455"), gfx::hex("#000022"), gfx::Align::Center);
}

void UIManager::drawEnd(sf::RenderTarget& rt, float fade) {
    if (fade > 0.f) gfx::px(rt, 0, 0, W, H, gfx::rgba(0, 0, 0, fade));
    if (fade < 0.96f) return;

    for (int i = 0; i < 8; ++i) {
        gfx::px(rt, 0, gfx::frand() * H, W, 2.f + gfx::frand() * 3.f, gfx::rgba(255, 0, 0, gfx::frand() * 0.12f));
    }
    for (float y = 0; y < H; y += 3.f) gfx::px(rt, 0, y, W, 1, gfx::rgba(0, 0, 0, 0.2f));

    gfx::plainText(rt, font, "DEMO COMPLETED", W / 2.f + 3.f, H / 2.f - 54.f, 42, gfx::hex("#220000"), gfx::Align::Center);
    gfx::plainText(rt, font, "DEMO COMPLETED", W / 2.f, H / 2.f - 56.f, 42, gfx::hex("#ff1111"), gfx::Align::Center);
    gfx::retroText(rt, font, "\u2014 THE PROTOCOL CONTINUES \u2014", W / 2.f, H / 2.f - 2.f, 17,
                   gfx::hex("#9999cc"), gfx::hex("#00002288"), gfx::Align::Center);
    gfx::retroText(rt, font, "SONIC.EXE: LAST PROTOCOL  \u00b7  FAN-GAME PROTOTYPE", W / 2.f, H / 2.f + 42.f, 10,
                   gfx::hex("#334455"), gfx::hex("#00002288"), gfx::Align::Center);
    gfx::retroText(rt, font, "[R] RESTART DEMO", W / 2.f, H / 2.f + 82.f, 13,
                   gfx::hex("#334455"), gfx::hex("#00002288"), gfx::Align::Center);
}
