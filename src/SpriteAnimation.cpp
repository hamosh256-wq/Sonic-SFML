#include "SpriteAnimation.hpp"
#include <utility>
#include <iostream>


// ═══════════════════════════════════════════ SpriteAnimation ══════════════════════════════════════════

SpriteAnimation::SpriteAnimation(std::vector<sf::IntRect> frames, float frameDuration, bool loop)
    : frames_(std::move(frames)), loop_(loop) {
    durations_.assign(frames_.size(), frameDuration);
}

SpriteAnimation::SpriteAnimation(std::vector<sf::IntRect> frames, std::vector<float> durations, bool loop)
    : frames_(std::move(frames)), durations_(std::move(durations)), loop_(loop) {
}

void SpriteAnimation::update(float dt) {
    if (frames_.empty() || finished_) return;
    elapsed_ += dt * speed_;
    // while (not if): a hitch that produces a large dt still lands on the
    // frame that would truly be current, instead of drifting one frame
    // behind forever.
    while (elapsed_ >= durations_[index_]) {
        elapsed_ -= durations_[index_];
        if (index_ + 1 < frames_.size()) {
            ++index_;
        } else if (loop_) {
            index_ = 0;
        } else {
            finished_ = true;
            break;
        }
    }
}

void SpriteAnimation::reset() {
    index_ = 0;
    elapsed_ = 0.f;
    finished_ = false;
}

void SpriteAnimation::setSpeed(float mult) { speed_ = mult; }

const sf::IntRect& SpriteAnimation::currentRect() const {
    static const sf::IntRect kEmpty{};
    if (frames_.empty()) return kEmpty;
    return frames_[index_];
}

// ═══════════════════════════════════════════ AnimationController ═══════════════════════════════════════════

AnimationController::AnimationController(const sf::Texture& texture) : sprite_(texture) {}

void AnimationController::addClip(const std::string& name, SpriteAnimation clip) {
    clips_[name] = std::move(clip); // overwrite-on-redefine, unlike emplace
}

bool AnimationController::play(const std::string& name, bool restart) {
    auto it = clips_.find(name);
    if (it == clips_.end()) return false;
    if (current_ == name && !restart) return true;

    current_ = name;
    it->second.reset();
    wasFinished_ = false;

    // Apply immediately so switching clips never shows one stale frame
    // of the previous animation before the next update() tick.
    const sf::IntRect& r = it->second.currentRect();
    sprite_.setTextureRect(r);
    sprite_.setOrigin({r.size.x / 2.f, (float)r.size.y});
    return true;
}

void AnimationController::update(float dt) {
    auto it = clips_.find(current_);
    if (it == clips_.end()) return;

    it->second.update(dt);

    const sf::IntRect& r = it->second.currentRect();

    static int counter = 0;
    if (++counter % 30 == 0) {
        std::cout << "Rect: "
                  << r.position.x << ", "
                  << r.position.y << "  "
                  << r.size.x << "x"
                  << r.size.y << '\n';
    }

    sprite_.setTextureRect(r);

    sprite_.setOrigin({r.size.x / 2.f, (float)r.size.y});

    bool fin = it->second.isFinished();
    if (fin && !wasFinished_) {
        wasFinished_ = true;
        if (onFinished) onFinished();
    } else if (!fin) {
        wasFinished_ = false;
    }
}

void AnimationController::setFacingRight(bool right) {
    if (facingRight_ == right) return;
    facingRight_ = right;
    sprite_.setScale({(right ? 1.f : -1.f) * baseScale_, baseScale_});
}

void AnimationController::setPosition(sf::Vector2f pos) { sprite_.setPosition(pos); }

void AnimationController::setScale(float uniformScale) {
    baseScale_ = uniformScale;
    sprite_.setScale({(facingRight_ ? 1.f : -1.f) * baseScale_, baseScale_});
}

bool AnimationController::currentFinished() const {
    auto it = clips_.find(current_);
    return it != clips_.end() && it->second.isFinished();
}
