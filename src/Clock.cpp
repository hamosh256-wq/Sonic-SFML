#include "Clock.hpp"
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

namespace clk {
namespace {
    sf::Clock g_clock; // starts counting the moment the program links this TU
}

float now() { return g_clock.getElapsedTime().asSeconds(); }
long long nowMs() { return g_clock.getElapsedTime().asMilliseconds(); }

} // namespace clk
