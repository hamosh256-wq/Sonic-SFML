#include "Input.hpp"

sf::Keyboard::Key Input::mapKey(GKey k) {
    switch (k) {
        case GKey::Left:   return sf::Keyboard::Key::Left;
        case GKey::Right:  return sf::Keyboard::Key::Right;
        case GKey::Up:     return sf::Keyboard::Key::Up;
        case GKey::A:      return sf::Keyboard::Key::A;
        case GKey::D:      return sf::Keyboard::Key::D;
        case GKey::W:      return sf::Keyboard::Key::W;
        case GKey::Space:  return sf::Keyboard::Key::Space;
        case GKey::ShiftL: return sf::Keyboard::Key::LShift;
        case GKey::ShiftR: return sf::Keyboard::Key::RShift;
        case GKey::Q:      return sf::Keyboard::Key::Q;
        case GKey::E:      return sf::Keyboard::Key::E;
        case GKey::R:      return sf::Keyboard::Key::R;
        case GKey::F:      return sf::Keyboard::Key::F;
        case GKey::Enter:  return sf::Keyboard::Key::Enter;
        default:           return sf::Keyboard::Key::Unknown;
    }
}

void Input::update() {
    prev_ = cur_;
    for (size_t i = 0; i < (size_t)GKey::Count; ++i) {
        cur_[i] = sf::Keyboard::isKeyPressed(mapKey((GKey)i));
    }
}
