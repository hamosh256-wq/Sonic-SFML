// ═══════════════════════════════════════════
// MAIN — window creation + frame loop
//
// ADAPTED for the unified project: Game's constructor now needs a
// const sf::Texture& for MetalSonicController's sprite. The texture is
// declared here, before `game`, so it's guaranteed to outlive it (C++
// destroys locals in reverse declaration order) - required since
// AnimationController only stores a reference to the texture, not a copy.
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <optional>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "Game.hpp"

constexpr float GAME_W = 960.f;
constexpr float GAME_H = 600.f;

int main() {
    std::srand(0); // deterministic-ish seed; swap for std::time(nullptr) if varied runs are preferred

    sf::RenderWindow window(sf::VideoMode({(unsigned)GAME_W, (unsigned)GAME_H}), "SONIC.EXE: LAST PROTOCOL");
    window.setFramerateLimit(0);

    // Declared BEFORE `game` - see comment above.
    sf::Texture metalTexture;
    if (!metalTexture.loadFromFile("assets/textures/metal_sonic_heroes.png")) {
        std::cerr << "Failed to load assets/textures/metal_sonic_heroes.png "
                     "(check the working directory the binary is launched from)\n";
        return 1;
    }

    Game game(GAME_W, GAME_H, "assets/fonts/LiberationMono-Regular.ttf", metalTexture);

    sf::Clock frameClock;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        float dt = std::min(frameClock.restart().asSeconds(), 0.034f);

        game.inp.update();
        game.update(dt);

        window.clear();
        game.draw(window);
        window.display();
    }

    return 0;
}
