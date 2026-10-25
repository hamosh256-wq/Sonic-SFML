#pragma once
// ═══════════════════════════════════════════
// CURSE SYSTEM
//
// A second damage channel, separate from HP/rings, that only
// reality-warping attacks feed. Two ways it resolves:
//   - Passively resisted: `resistance` mitigates how much of each hit
//     actually lands, and it bleeds off on its own over time (Metal
//     Sonic's own systems fighting the intrusion in the background).
//   - Actively channeled: MetalSonicController can `vent()` banked
//     corruption as fuel for the Super/Hyper transformation instead of
//     just enduring it - mechanically, "resist" and "embrace" draw from
//     the same meter.
//
// Deliberately has no idea that Sonic.exe, Metal Sonic, or a state
// machine exist - it's a plain resource, so it's just as usable for any
// future character who needs to resist/channel the same corruption.
// ═══════════════════════════════════════════
#include <algorithm>

class CurseState {
public:
    explicit CurseState(float maxCorruption = 100.f, float resistance = 1.f)
        : maxCorruption_(maxCorruption), resistance_(resistance) {}

    // A reality-warping attack connects. `resistance` divides down how
    // much of `rawPower` actually lands - an upgradeable stat, not a
    // flat block.
    void exposeToCorruption(float rawPower) {
        float mitigated = rawPower / std::max(0.1f, resistance_);
        corruption_ = std::min(maxCorruption_, corruption_ + mitigated);
    }

    // Passive decay - call once per frame regardless of state.
    void update(float dt) {
        corruption_ = std::max(0.f, corruption_ - passiveDecayRate_ * dt);
    }

    // Spends banked corruption as transformation fuel. Returns the
    // amount actually vented (clamped to what's banked, so callers can
    // tell if they came up short of a requested cost).
    float vent(float amount) {
        float vented = std::min(amount, corruption_);
        corruption_ -= vented;
        return vented;
    }

    bool isOverwhelmed() const { return corruption_ >= maxCorruption_; }
    bool canChannel(float threshold) const { return corruption_ >= threshold; }

    float corruption() const { return corruption_; }
    float corruption01() const { return corruption_ / maxCorruption_; }
    float maxCorruption() const { return maxCorruption_; }

    void setResistance(float r) { resistance_ = std::max(0.f, r); }
    float resistance() const { return resistance_; }
    void setPassiveDecayRate(float perSecond) { passiveDecayRate_ = std::max(0.f, perSecond); }

private:
    float corruption_ = 0.f;
    float maxCorruption_;
    float resistance_;
    float passiveDecayRate_ = 6.f; // corruption/sec drained when not actively hit
};
