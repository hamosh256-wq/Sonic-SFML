#pragma once
// ═══════════════════════════════════════════
// CORRUPTION DIRECTOR — the "self-destruct" story beat
//
// DESIGN NOTE (read this before wiring it up):
// This subscribes to the "hyper_vs_exe" event MetalSonicController's
// Hyper state emits and plays out entirely as rendered SFML content -
// screen tearing, fake system-log text, a hard cut to black, then a
// scripted aftermath screen. It never touches the filesystem: no
// deleting, renaming, or overwriting anything, the player's real save
// data included. Everything "corrupted" here is a fictional in-universe
// filename I made up for flavor (CORE_MEMORY.SYS, REALITY_ANCHOR.DLL,
// etc.) — it never references your project's actual file names, so
// there's no ambiguity for the player about whether something real just
// broke.
//
// This is a deliberate choice, not a simplification of what you asked
// for: actually deleting/corrupting files on the player's machine is
// real destructive behavior against their system, and the illusion is
// strictly better even from a pure game-feel standpoint - the player
// only ever experiences a horror beat through what's rendered on screen
// and how the game behaves, so faking it convincingly gets 100% of the
// scare with none of the risk (antivirus false-positives, real data
// loss if a bug in "real" corruption logic ever touched the wrong path,
// App Store/Steam review rejection, etc). Same reasoning as how Doki
// Doki Literature Club, Undertale, and Pony Island all pull off "the
// game is breaking" without harming anything on your actual disk.
//
// It also deliberately does NOT call window.close()/std::exit() when
// the sequence ends - forcibly closing the whole application is a real,
// disruptive action that should be an explicit choice your Game class
// makes (e.g. drop to a restart screen, like the existing ending flow),
// not something this component decides unilaterally.
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "EventBus.hpp"

class CorruptionDirector {
public:
    CorruptionDirector(EventBus& ev, sf::Font& font, sf::Vector2u screenSize);

    void update(float dt);
    void draw(sf::RenderTarget& rt) const;

    bool isActive() const { return stage_ != Stage::Idle && stage_ != Stage::Done; }
    bool isFinished() const { return stage_ == Stage::Done; } // Game can act on this (e.g. offer restart)

    // Lets Game trigger it directly too (e.g. a debug key, or a narrative
    // path that doesn't go through MetalSonicController at all).
    void trigger();

    // Re-arms the director after a restart so a future Hyper-vs-.exe can
    // trigger the sequence again. No-op while a sequence is mid-flight.
    void resetToIdle() { if (stage_ == Stage::Done) enterStage(Stage::Idle); }

private:
    enum class Stage { Idle, Flicker, ErrorLog, FileScramble, HardCut, Aftermath, Done };

    void enterStage(Stage s);

    Stage stage_ = Stage::Idle;
    float stageT_ = 0.f;
    sf::Font& font_;
    sf::Vector2u screen_;
    std::vector<std::string> logLines_;   // fictional, revealed one at a time during ErrorLog/FileScramble
    std::size_t linesShown_ = 0;
    float shake_ = 0.f;
};
