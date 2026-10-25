#pragma once
// ═══════════════════════════════════════════
// AABB COLLISION
// Identical to the one already in your Sonic.EXE: Last Protocol project.
// PhysicsBody.hpp's fields (x,y,vx,vy,w,h,onGround) are named to match
// this template's duck-typing requirements exactly, so resolveAABB works
// unchanged against the new physics component.
// ═══════════════════════════════════════════
#include <vector>
#include <algorithm>

struct Platform {
    float x, y, w, h;
};

template <typename Body>
inline void resolveAABB(Body& e, const std::vector<Platform>& plats) {
    e.onGround = false;
    float hw = e.w / 2.f;
    for (const auto& p : plats) {
        float eL = e.x - hw, eR = e.x + hw, eT = e.y - e.h, eB = e.y;
        if (eR <= p.x || eL >= p.x + p.w || eB <= p.y || eT >= p.y + p.h) continue;

        float ovT = eB - p.y;
        float ovB = (p.y + p.h) - eT;
        float ovL = eR - p.x;
        float ovR = (p.x + p.w) - eL;

        if (std::min(ovT, ovB) <= std::min(ovL, ovR)) {
            if (ovT < ovB) {
                e.y = p.y;
                e.vy = std::min(e.vy, 0.f);
                e.onGround = true;
            } else {
                e.y = p.y + p.h + e.h;
                if (e.vy < 0.f) e.vy = 0.f;
            }
        } else {
            if (ovL < ovR) { e.x = p.x - hw; e.vx = 0.f; }
            else { e.x = p.x + p.w + hw; e.vx = 0.f; }
        }
    }
}
