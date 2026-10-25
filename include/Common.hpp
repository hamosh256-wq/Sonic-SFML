#pragma once
// ═══════════════════════════════════════════
// COMMON / RETRO RENDERING UTILITIES
// Identical to the one already in your Sonic.EXE: Last Protocol project.
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace gfx {

enum class Align { Left, Center, Right };

inline sf::Color hex(const std::string& s) {
    std::string h = s;
    if (!h.empty() && h[0] == '#') h = h.substr(1);
    unsigned r = 255, g = 255, b = 255, a = 255;
    if (h.size() >= 6) {
        r = std::stoul(h.substr(0, 2), nullptr, 16);
        g = std::stoul(h.substr(2, 2), nullptr, 16);
        b = std::stoul(h.substr(4, 2), nullptr, 16);
    }
    if (h.size() >= 8) a = std::stoul(h.substr(6, 2), nullptr, 16);
    return sf::Color((std::uint8_t)r, (std::uint8_t)g, (std::uint8_t)b, (std::uint8_t)a);
}

inline sf::Color hexA(const std::string& s, float alpha) {
    sf::Color c = hex(s);
    c.a = (std::uint8_t)(std::clamp(alpha, 0.f, 1.f) * 255.f);
    return c;
}

inline sf::Color rgba(int r, int g, int b, float a) {
    return sf::Color((std::uint8_t)r, (std::uint8_t)g, (std::uint8_t)b,
                      (std::uint8_t)(std::clamp(a, 0.f, 1.f) * 255.f));
}

inline sf::Color hsl(float h, float s, float l, float a = 1.f) {
    h = std::fmod(h, 360.f);
    if (h < 0) h += 360.f;
    float c = (1.f - std::fabs(2.f * l - 1.f)) * s;
    float x = c * (1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f));
    float m = l - c / 2.f;
    float r, g, b;
    if (h < 60)        { r = c; g = x; b = 0; }
    else if (h < 120)  { r = x; g = c; b = 0; }
    else if (h < 180)  { r = 0; g = c; b = x; }
    else if (h < 240)  { r = 0; g = x; b = c; }
    else if (h < 300)  { r = x; g = 0; b = c; }
    else               { r = c; g = 0; b = x; }
    return sf::Color((std::uint8_t)((r + m) * 255.f), (std::uint8_t)((g + m) * 255.f),
                      (std::uint8_t)((b + m) * 255.f), (std::uint8_t)(a * 255.f));
}

inline float frand() { return (float)std::rand() / (float)RAND_MAX; }
inline float frand(float lo, float hi) { return lo + frand() * (hi - lo); }
inline float sign(float v) { return (float)((v > 0.f) - (v < 0.f)); }

inline std::string padStart(long long value, int width) {
    std::string s = std::to_string(value);
    if (value < 0) return s;
    while ((int)s.size() < width) s = "0" + s;
    return s;
}

inline sf::String toSfString(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

inline float textWidth(sf::Font& font, const std::string& text, unsigned size, bool bold = false) {
    sf::Text t(font, toSfString(text), size);
    if (bold) t.setStyle(sf::Text::Style::Bold);
    return t.getLocalBounds().size.x;
}

inline void px(sf::RenderTarget& rt, float x, float y, float w, float h, sf::Color col) {
    sf::RectangleShape r({std::max(1.f, std::round(w)), std::max(1.f, std::round(h))});
    r.setPosition({std::round(x), std::round(y)});
    r.setFillColor(col);
    rt.draw(r);
}

inline void thickLine(sf::RenderTarget& rt, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color col) {
    sf::Vector2f d = b - a;
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 0.0001f) return;
    sf::RectangleShape line({len, std::max(1.f, thickness)});
    line.setFillColor(col);
    line.setOrigin({0.f, thickness / 2.f});
    line.setPosition(a);
    line.setRotation(sf::radians(std::atan2(d.y, d.x)));
    rt.draw(line);
}

inline void strokeCircle(sf::RenderTarget& rt, sf::Vector2f center, float radius, float thickness, sf::Color col) {
    sf::CircleShape c(radius, 32);
    c.setOrigin({radius, radius});
    c.setPosition(center);
    c.setFillColor(sf::Color::Transparent);
    c.setOutlineColor(col);
    c.setOutlineThickness(thickness);
    rt.draw(c);
}

inline void fillCircle(sf::RenderTarget& rt, sf::Vector2f center, float radius, sf::Color col, std::size_t pts = 24) {
    sf::CircleShape c(radius, pts);
    c.setOrigin({radius, radius});
    c.setPosition(center);
    c.setFillColor(col);
    rt.draw(c);
}

inline void fillEllipse(sf::RenderTarget& rt, sf::Vector2f center, float rx, float ry, sf::Color col, float rotationRad = 0.f, std::size_t pts = 24) {
    sf::CircleShape c(1.f, pts);
    c.setScale({rx, ry});
    c.setOrigin({1.f, 1.f});
    c.setPosition(center);
    if (rotationRad != 0.f) c.setRotation(sf::radians(rotationRad));
    c.setFillColor(col);
    rt.draw(c);
}

inline void radialGlow(sf::RenderTarget& rt, sf::Vector2f center, float radius, sf::Color innerCol, sf::Color outerCol, int rings = 5) {
    for (int i = rings; i >= 1; --i) {
        float t = (float)i / (float)rings;
        sf::Color c;
        c.r = (std::uint8_t)(outerCol.r + (innerCol.r - outerCol.r) * (1.f - t));
        c.g = (std::uint8_t)(outerCol.g + (innerCol.g - outerCol.g) * (1.f - t));
        c.b = (std::uint8_t)(outerCol.b + (innerCol.b - outerCol.b) * (1.f - t));
        c.a = (std::uint8_t)((outerCol.a + (innerCol.a - outerCol.a) * (1.f - t)) * (1.f / rings) * 1.6f);
        fillCircle(rt, center, radius * t, c, 24);
    }
}

inline sf::Vector2f rotatePoint(sf::Vector2f p, float angleRad) {
    float c = std::cos(angleRad), s = std::sin(angleRad);
    return {p.x * c - p.y * s, p.x * s + p.y * c};
}

inline void pieSlice(sf::RenderTarget& rt, sf::Vector2f center, float radius,
                      float startDeg, float endDeg, sf::Color col, int segments = 12) {
    sf::ConvexShape fan(segments + 2);
    fan.setPoint(0, {0.f, 0.f});
    float startRad = startDeg * 3.14159265f / 180.f;
    float endRad = endDeg * 3.14159265f / 180.f;
    for (int i = 0; i <= segments; ++i) {
        float a = startRad + (endRad - startRad) * (float)i / (float)segments;
        fan.setPoint(1 + i, {std::cos(a) * radius, std::sin(a) * radius});
    }
    fan.setFillColor(col);
    fan.setPosition(center);
    rt.draw(fan);
}

inline sf::Color mulAlpha(sf::Color c, float factor) {
    c.a = (std::uint8_t)(std::clamp((float)c.a * factor, 0.f, 255.f));
    return c;
}

inline void strokeRect(sf::RenderTarget& rt, float x, float y, float w, float h, float thickness, sf::Color col) {
    sf::RectangleShape r({w, h});
    r.setPosition({x, y});
    r.setFillColor(sf::Color::Transparent);
    r.setOutlineColor(col);
    r.setOutlineThickness(thickness);
    rt.draw(r);
}

inline void retroText(sf::RenderTarget& rt, sf::Font& font, const std::string& text,
                       float x, float y, unsigned size, sf::Color col,
                       sf::Color shadow = sf::Color(0, 0, 0, 136),
                       Align align = Align::Left, bool bold = true,
                       float shadowOffset = 2.f) {
    sf::Text t(font, toSfString(text), size);
    if (bold) t.setStyle(sf::Text::Style::Bold);
    sf::FloatRect b = t.getLocalBounds();
    float ox = b.position.x;
    if (align == Align::Center) ox = b.position.x + b.size.x / 2.f;
    else if (align == Align::Right) ox = b.position.x + b.size.x;
    t.setOrigin({ox, 0.f});

    t.setFillColor(shadow);
    t.setPosition({x + shadowOffset, y + shadowOffset});
    rt.draw(t);

    t.setFillColor(col);
    t.setPosition({x, y});
    rt.draw(t);
}

inline void plainText(sf::RenderTarget& rt, sf::Font& font, const std::string& text,
                       float x, float y, unsigned size, sf::Color col,
                       Align align = Align::Left, bool bold = true) {
    sf::Text t(font, toSfString(text), size);
    if (bold) t.setStyle(sf::Text::Style::Bold);
    sf::FloatRect b = t.getLocalBounds();
    float ox = b.position.x;
    if (align == Align::Center) ox = b.position.x + b.size.x / 2.f;
    else if (align == Align::Right) ox = b.position.x + b.size.x;
    t.setOrigin({ox, 0.f});
    t.setFillColor(col);
    t.setPosition({x, y});
    rt.draw(t);
}

inline sf::View pushClip(sf::RenderTarget& rt, float x, float y, float w, float h) {
    sf::View old = rt.getView();
    sf::Vector2u sz = rt.getSize();
    float vx = std::clamp(x / (float)sz.x, 0.f, 1.f);
    float vy = std::clamp(y / (float)sz.y, 0.f, 1.f);
    float vw = std::clamp((x + w) / (float)sz.x, 0.f, 1.f) - vx;
    float vh = std::clamp((y + h) / (float)sz.y, 0.f, 1.f) - vy;
    sf::View clip(sf::FloatRect({x, y}, {w, h}));
    clip.setViewport(sf::FloatRect({vx, vy}, {std::max(0.f, vw), std::max(0.f, vh)}));
    rt.setView(clip);
    return old;
}
inline void popClip(sf::RenderTarget& rt, const sf::View& old) { rt.setView(old); }

} // namespace gfx
