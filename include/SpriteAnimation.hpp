#pragma once
// ═══════════════════════════════════════════
// SPRITE ANIMATION
//
// Two layers, deliberately kept separate:
//   - SpriteAnimation:   one clip's playhead (frame list + timing + loop).
//   - AnimationController: owns the sf::Sprite + texture binding, holds a
//     named map of clips, and switches between them on command.
//
// Both advance purely off dt (no fixed-frame-rate assumption anywhere),
// so playback speed stays correct whether the game runs at 30, 60, or
// 240 fps, and `setSpeed()` lets gameplay (e.g. Overdrive) scale
// animation rate independently of game logic.
// ═══════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

class SpriteAnimation {
public:
    SpriteAnimation() = default;

    // Uniform per-frame duration (the common case).
    SpriteAnimation(std::vector<sf::IntRect> frames, float frameDuration, bool loop = true);

    // Per-frame durations (e.g. a held final "impact" frame that lingers
    // longer than the rest — sizes of `frames` and `durations` must match).
    SpriteAnimation(std::vector<sf::IntRect> frames, std::vector<float> durations, bool loop = true);

    // Advances the playhead by dt * speed. Safe against large dt spikes:
    // uses a while-loop so a hitch never leaves the animation "owing" time,
    // it just catches up to the correct frame immediately.
    void update(float dt);

    void reset();                    // rewind to frame 0, clear finished flag
    void setSpeed(float mult);       // 1.0 = normal; Overdrive can push this >1
    void setLoop(bool loop) { loop_ = loop; }

    const sf::IntRect& currentRect() const;
    std::size_t currentIndex() const { return index_; }
    std::size_t frameCount() const { return frames_.size(); }
    bool isFinished() const { return finished_; } // only ever true for loop=false clips
    bool empty() const { return frames_.empty(); }

private:
    std::vector<sf::IntRect> frames_;
    std::vector<float> durations_;
    std::size_t index_ = 0;
    float elapsed_ = 0.f;
    float speed_ = 1.f;
    bool loop_ = true;
    bool finished_ = false;
};

// ─────────────────────────────────────────────────────────────
// AnimationController — per-entity: owns the drawable sprite,
// holds named clips, switches between them, applies flip/position.
// ─────────────────────────────────────────────────────────────
class AnimationController {
public:
    // SFML3's sf::Sprite has no default constructor - it must bind to a
    // texture immediately, so AnimationController requires one up front.
    // The texture must outlive this controller (own it in the character
    // class or a shared TextureManager, not as a local temporary).
    explicit AnimationController(const sf::Texture& texture);

    void addClip(const std::string& name, SpriteAnimation clip);

    // Switches to `name` only if it isn't already the active clip, unless
    // restart=true. Returns false silently if the name isn't registered
    // (keeps calling code simple; check hasClip() first if that matters).
    bool play(const std::string& name, bool restart = false);

    void update(float dt);

    void setFacingRight(bool right); // flips the sprite about its own center
    void setPosition(sf::Vector2f pos);
    void setScale(float uniformScale);

    bool hasClip(const std::string& name) const { return clips_.count(name) > 0; }
    const std::string& currentClipName() const { return current_; }
    bool currentFinished() const;

    // Fires once, the frame a non-looping clip completes. Reassign per
    // state as needed (e.g. Character sets this in its Attack state's
    // enter() and clears it in exit()).
    std::function<void()> onFinished;

    const sf::Sprite& sprite() const { return sprite_; }
    sf::Sprite& sprite() { return sprite_; }

private:
    sf::Sprite sprite_;
    std::unordered_map<std::string, SpriteAnimation> clips_;
    std::string current_;
    bool facingRight_ = true;
    float baseScale_ = 1.f;
    bool wasFinished_ = false;
};
