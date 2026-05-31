#pragma once

#include <hyprutils/animation/AnimationManager.hpp>

#include <chrono>

namespace Hyprutils::Animation {
    void advanceSpring(float& value, float& velocity, const SSpringCurve& spring, std::chrono::duration<float> elapsed);
}
