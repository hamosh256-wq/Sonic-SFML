#pragma once
// ═══════════════════════════════════════════
// EVENT BUS
// Identical to the one already in your Sonic.EXE: Last Protocol project.
// Included here so this module compiles standalone; safe to skip
// overwriting if you're dropping these files into the existing project.
// ═══════════════════════════════════════════
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

struct EventData {
    float x = 0.f, y = 0.f;
};

class EventBus {
public:
    using Callback = std::function<void(const EventData&)>;

    void on(const std::string& name, Callback cb) {
        listeners_[name].push_back(std::move(cb));
    }

    void emit(const std::string& name, const EventData& data = {}) {
        auto it = listeners_.find(name);
        if (it == listeners_.end()) return;
        auto snapshot = it->second;
        for (auto& fn : snapshot) fn(data);
    }

private:
    std::unordered_map<std::string, std::vector<Callback>> listeners_;
};
