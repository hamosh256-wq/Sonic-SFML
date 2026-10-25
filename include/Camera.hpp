#pragma once
// ═══════════════════════════════════════════
// CAMERA (intensity-aware shake)
// ═══════════════════════════════════════════

class Camera {
public:
    Camera(float W, float H);

    void follow(float tx, float ty, float lw, float lh);
    void shake(float intensity = 18.f, float dur = 0.4f);

    float wx(float worldX) const { return worldX - x_ - sx_; }
    float wy(float worldY) const { return worldY - y_ - sy_; }

    float x() const { return x_; }
    float y() const { return y_; }
    void setX(float v) { x_ = v; }
    void setY(float v) { y_ = v; }

private:
    float W_, H_;
    float x_ = 0.f, y_ = 0.f;
    float sx_ = 0.f, sy_ = 0.f;
    float sd_ = 0.f;
    float si_ = 0.f;
};
