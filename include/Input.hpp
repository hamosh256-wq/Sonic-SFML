#pragma once
// ═══════════════════════════════════════════
// INPUT — identical to the one already in your project.
// ═══════════════════════════════════════════
#include <SFML/Window/Keyboard.hpp>
#include <array>
#include <cstddef>

enum class GKey {
    Left, Right, Up, A, D, W, Space, ShiftL, ShiftR, Q, E, R, F, Enter,
    Count
};

class Input {
public:
    void update();
    bool down(GKey k) const { return cur_[(size_t)k]; }
    bool just(GKey k) const { return cur_[(size_t)k] && !prev_[(size_t)k]; }

private:
    std::array<bool, (size_t)GKey::Count> cur_{};
    std::array<bool, (size_t)GKey::Count> prev_{};
    static sf::Keyboard::Key mapKey(GKey k);
};
