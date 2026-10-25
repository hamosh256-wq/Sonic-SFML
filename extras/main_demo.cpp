// ═══════════════════════════════════════════
// DEMO — proves the whole system out end-to-end
//
// A flat test arena, no enemies. Debug keys stand in for a real Sonic.exe
// entity so you can exercise the full Curse -> Phantom Ruby -> Super ->
// Hyper -> corruption-sequence pipeline without having ported SonicExe
// into this module too:
//
//   Left/Right or A/D   - move            Space/Up/W - jump
//   Hold F              - charge Phantom Ruby (needs curse >= 40)
//     -> hold to completion = Super. Hold F again while in Super
//        AND vs-EXE is toggled on AND curse refilled >= 70 = Hyper,
//        which fires the corruption sequence.
//   C                   - simulate a reality-warping hit (+18 curse)
//   V                   - toggle "fighting the .exe" context
//   R                   - reset (also restarts after the corruption
//                         sequence finishes)
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <algorithm>
#include <string>
#include "EventBus.hpp"
#include "Input.hpp"
#include "Physics.hpp"
#include "PhysicsBody.hpp"
#include "MetalSonicController.hpp"
#include "CorruptionDirector.hpp"
#include "Common.hpp"

constexpr float W = 960.f, H = 540.f;

int main() {
    sf::RenderWindow window(sf::VideoMode({(unsigned)W, (unsigned)H}), "Metal Sonic Control System - Demo");
    window.setFramerateLimit(0);

    sf::Font font;
    if (!font.openFromFile("assets/fonts/LiberationMono-Regular.ttf")) {
        // Non-fatal: text just won't render. Path is relative to the
        // working directory the binary is launched from.
    }

    // NOTE ON THIS LOAD CALL: written against sf::Texture::loadFromFile
    // returning bool, which is correct for the SFML 3.0 release line as
    // I know it. If your specific SFML3 build has moved Texture to the
    // std::optional-returning factory style (a couple of resource types
    // changed shape between SFML3 pre-releases), swap this for:
    //   auto loaded = sf::Texture::loadFromFile("...");
    //   if (!loaded) { ... } else { metalTexture = std::move(*loaded); }
    sf::Texture metalTexture;
    if (!metalTexture.loadFromFile("assets/textures/metal_sonic_heroes.png")) {
        return 1; // can't run the demo without it - check the working directory
    }

    EventBus eventBus;
    std::vector<Platform> ground = { {0.f, 480.f, W, 60.f} };

    MetalSonicController metal(metalTexture, W / 2.f, 480.f, eventBus);
    CorruptionDirector director(eventBus, font, {(unsigned)W, (unsigned)H});

    Input input;
    sf::Clock frameClock;
    bool vsEXE = false;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        float dt = std::min(frameClock.restart().asSeconds(), 0.034f);
        input.update();

        if (input.just(GKey::R)) {
            metal.reset(W / 2.f, 480.f);
            director.resetToIdle();
            vsEXE = false;
        }

        static bool cPrev = false, vPrev = false;
        bool cNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C);
        bool vNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::V);
        if (cNow && !cPrev) metal.exposeToCorruption(18.f);
        if (vNow && !vPrev) { vsEXE = !vsEXE; metal.setFacingCorruptionEntity(vsEXE); }
        cPrev = cNow; vPrev = vNow;

        metal.update(dt, input, ground);
        director.update(dt);

        window.clear(sf::Color(10, 10, 20));
        for (const auto& p : ground) gfx::px(window, p.x, p.y, p.w, p.h, gfx::hex("#223355"));
        metal.draw(window);

        // Debug HUD
        gfx::retroText(window, font, "FORM: " + metal.formState(), 12, 12, 12, sf::Color::White);
        gfx::retroText(window, font, "CURSE: " + std::to_string((int)(metal.curse01() * 100)) + "%",
                       12, 30, 12, gfx::hex("#ff6666"));
        gfx::retroText(window, font, std::string("VS .EXE: ") + (vsEXE ? "YES" : "no"), 12, 48, 12,
                       vsEXE ? gfx::hex("#ff3333") : gfx::hex("#666688"));
        if (metal.isGlitched()) {
            gfx::retroText(window, font, "SYSTEMS OVERWHELMED", W / 2.f, 12, 12, gfx::hex("#ff3333"),
                           sf::Color::Black, gfx::Align::Center);
        }

        director.draw(window); // on top of everything once triggered
        window.display();
    }
    return 0;
}
