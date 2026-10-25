#pragma once
// ═══════════════════════════════════════════
// STATE MACHINE
// Identical to the one already in your Sonic.EXE: Last Protocol project.
// The Curse/Super/Hyper FSM below (CharacterFSM) is built on this exact
// same StateDef{enter,exit,update} pattern for consistency.
// ═══════════════════════════════════════════
#include <functional>
#include <unordered_map>
#include <string>
#include <initializer_list>

struct StateDef {
    std::function<void()> enter;
    std::function<void()> exit;
    std::function<void(float dt, float t)> update;
};

class StateMachine {
public:
    void init(std::unordered_map<std::string, StateDef> states, const std::string& initial) {
        states_ = std::move(states);
        cur_ = initial;
        t_ = 0.f;
        auto it = states_.find(cur_);
        if (it != states_.end() && it->second.enter) it->second.enter();
    }

    void go(const std::string& next) {
        if (next == cur_) return;
        auto nit = states_.find(next);
        if (nit == states_.end()) return;
        auto cit = states_.find(cur_);
        if (cit != states_.end() && cit->second.exit) cit->second.exit();
        cur_ = next;
        t_ = 0.f;
        if (nit->second.enter) nit->second.enter();
    }

    void tick(float dt) {
        t_ += dt;
        auto it = states_.find(cur_);
        if (it != states_.end() && it->second.update) it->second.update(dt, t_);
    }

    bool is(const std::string& name) const { return cur_ == name; }

    bool any(std::initializer_list<std::string> names) const {
        for (const auto& n : names) if (n == cur_) return true;
        return false;
    }

    const std::string& current() const { return cur_; }
    float t() const { return t_; }

private:
    std::unordered_map<std::string, StateDef> states_;
    std::string cur_;
    float t_ = 0.f;
};
