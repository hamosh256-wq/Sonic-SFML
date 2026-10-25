#include "Camera.hpp"
#include <algorithm>
#include "Common.hpp"

Camera::Camera(float W, float H) : W_(W), H_(H) {}

void Camera::follow(float tx, float ty, float lw, float lh) {
    x_ += (tx - W_ / 2.f - x_) * 0.11f;
    y_ += (ty - H_ / 2.f - 30.f - y_) * 0.11f;
    x_ = std::max(0.f, std::min(x_, lw - W_));
    y_ = std::max(0.f, std::min(y_, lh - H_));

    if (sd_ > 0.f) {
        sd_ -= 0.016f;
        float i = std::max(0.f, sd_) * si_;
        sx_ = (gfx::frand() - 0.5f) * i;
        sy_ = (gfx::frand() - 0.5f) * i;
    } else {
        sx_ = 0.f;
        sy_ = 0.f;
    }
}

void Camera::shake(float intensity, float dur) {
    si_ = intensity;
    sd_ = std::max(sd_, dur);
}
