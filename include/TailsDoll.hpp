#pragma once
// ═══════════════════════════════════════════
// TAILS DOLL — pixel art with Phantom Ruby
// `activate(t)` is templated so it accepts anything exposing public
// float x,y members - Game::_goPhase3 calls `tailsDoll.activate(this.
// robotnik.get())`, NOT Metal Sonic, so this can't be hard-typed to a
// single character class.
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include "Camera.hpp"

class TailsDoll {
public:
    template <typename T>
    void activate(T* target) {
        active = true;
        followX_ = &target->x;
        followY_ = &target->y;
    }
    void activateRuby();
    void update(float dt);
    void draw(sf::RenderTarget& rt, const Camera& cam);

    float x = 0.f, y = 0.f;
    bool active = false;
    float floatT = 0.f, gemT = 0.f;
    bool rubyOn = false;
    float rubyPow = 0.f;

private:
    float* followX_ = nullptr;
    float* followY_ = nullptr;
};
