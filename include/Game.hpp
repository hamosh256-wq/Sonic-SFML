#pragma once
// ═══════════════════════════════════════════
// GAME — main orchestrator
//
// ADAPTED for the unified project:
//   - metal_ is now a MetalSonicController, constructed ONCE in Game's
//     own constructor (it needs a texture reference at construction,
//     unlike the old MetalSonic). Every level load / restart calls
//     metal_->reset(x, y) instead of reconstructing it - reconstruction
//     would not compile anyway, since MetalSonicController holds an
//     EventBus& reference member and is deliberately not
//     copy/move-assignable. This is requirement #2 from the unification
//     ask, made structurally impossible to get wrong rather than just
//     documented.
//   - A CorruptionDirector member owns the "Hyper vs .exe" self-destruct
//     sequence; it self-subscribes to "hyper_vs_exe" in its own
//     constructor, so Game doesn't need to forward anything to it beyond
//     calling update()/draw() every frame.
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>
#include "EventBus.hpp"
#include "Input.hpp"
#include "Camera.hpp"
#include "UIManager.hpp"
#include "Particles.hpp"
#include "Explosion.hpp"
#include "ImpactStar.hpp"
#include "MetalSonicController.hpp"
#include "SonicExe.hpp"
#include "DrRobotnik.hpp"
#include "TailsDoll.hpp"
#include "Robot.hpp"
#include "RescueTarget.hpp"
#include "Level.hpp"
#include "CorruptionDirector.hpp"

struct StarDef {
    float x, y, sz, bright, spd, twinkle;
    sf::Color col;
};

struct BgGear {
    float bx, by, r;
    int teeth;
    float rot, spd;
};

class Game {
public:
    Game(float W, float H, const std::string& fontPath, const sf::Texture& metalTexture);

    void update(float dt);
    void draw(sf::RenderTarget& rt);
    void reset(); // full game reset (title screen) - does NOT touch metal_'s identity, only its state via reset()

    Input inp;

private:
    void wireEvents();
    void loadLevel(const std::string& key);
    void goPhase1();
    void goPhase2();
    void goPhase3();
    void goBoss1();
    void goBoss2();
    void triggerEnding();
    void tickTrans(float dt);

    void drawBg(sf::RenderTarget& rt);
    void drawGear(sf::RenderTarget& rt, float gx, float gy, float r, int teeth, float rot);
    void drawPlats(sf::RenderTarget& rt);
    void drawDistortion(sf::RenderTarget& rt);
    void drawEndingHeroes(sf::RenderTarget& rt);

    float W_, H_;
    EventBus ev_;
    Camera cam_;
    UIManager ui_;
    Particles ptcl_;
    CorruptionDirector director_;

    std::unique_ptr<MetalSonicController> metal_; // constructed once; see class comment above
    std::unique_ptr<SonicExe> exe_;
    std::unique_ptr<DrRobotnik> robotnik_;
    std::unique_ptr<TailsDoll> tailsDoll_;
    std::vector<Robot> enemies_;
    std::vector<Robot> allies_;
    std::unique_ptr<RescueTarget> rescueTgt_;
    const LevelData* level_ = nullptr;

    std::vector<Explosion> explosions_;
    std::vector<ImpactStar> impactStars_;
    float hitStop_ = 0.f;

    int score_ = 0;
    float gameTime_ = 0.f;
    std::string phase_ = "title";
    std::string phaseName_;
    bool gameOver_ = false;

    float transT_ = -1.f;
    std::string transNext_;

    bool endActive_ = false;
    float endT_ = 0.f;
    int endStep_ = 0;
    float endFade_ = 0.f;

    std::vector<StarDef> stars_;
    std::vector<BgGear> bgGears_;
};
