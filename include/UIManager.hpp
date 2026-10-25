#pragma once
// ═══════════════════════════════════════════
// UI MANAGER — HUD, dialogue box, title/game-over/ending screens
//
// ADAPTED for the unified project: the old 4-icon Q/E/R/F ability HUD
// (Shield/Repair/Drone/Overdrive) doesn't apply to MetalSonicController
// at all - those abilities never existed on it, it has Curse/Phantom
// Ruby/Super/Hyper instead. drawAbilityHUD() is replaced with
// drawFormHUD(), showing the curse meter and current form state.
// Everything else (title/dialog/game-over/ending/notifications) is
// unchanged.
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <unordered_map>
#include "MetalSonicController.hpp"
#include "SonicExe.hpp"
#include "DrRobotnik.hpp"

struct Notif {
    std::string text;
    float dur, t;
    sf::Color col;
    float oy;
};

class UIManager {
public:
    UIManager(float W, float H, const std::string& fontPath);

    void notify(const std::string& text, float dur = 2.5f, sf::Color col = sf::Color(0, 230, 118));
    void queueDlg(const std::vector<std::string>& lines);
    void advDlg();
    bool hasDlg() const { return curDlg_.has_value(); }
    void flashKey(const std::string& k);
    void update(float dt);

    void draw(sf::RenderTarget& rt, MetalSonicController* metal, SonicExe* exe, DrRobotnik* robotnik,
              bool isBoss, const std::string& phaseName, int score, float gameTime);
    void drawTitle(sf::RenderTarget& rt);
    void drawGameOver(sf::RenderTarget& rt);
    void drawEnd(sf::RenderTarget& rt, float fade);

    float W, H;
    sf::Font font;

private:
    void drawBossBar(sf::RenderTarget& rt, float hp, float maxHp);
    void drawDocBar(sf::RenderTarget& rt, float hp, float maxHp);
    void drawFormHUD(sf::RenderTarget& rt, MetalSonicController* m);
    void drawDialog(sf::RenderTarget& rt, const std::string& text);

    std::vector<Notif> notifs_;
    std::deque<std::string> dlgQueue_;
    std::optional<std::string> curDlg_;
    float dlgT_ = 0.f;
    std::unordered_map<std::string, float> flash_;

    void nextDlg();
};
