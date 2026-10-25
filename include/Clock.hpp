#pragma once
// ═══════════════════════════════════════════
// CLOCK — global elapsed-time stopwatch
// API-compatible with the clk::now()/clk::nowMs() calls already used
// throughout your project (continuous float seconds for sin()-driven
// flicker/pulse effects, integer ms for blink-toggle checks).
// ═══════════════════════════════════════════

namespace clk {
    float now();      // seconds since program start, monotonically increasing
    long long nowMs(); // milliseconds since program start
}
