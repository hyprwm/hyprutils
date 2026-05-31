#include <hyprutils/animation/Spring.hpp>

#include <algorithm>
#include <cmath>

using namespace Hyprutils;
using namespace Hyprutils::Animation;

void Animation::advanceSpring(float& value, float& velocity, const SSpringCurve& spring, std::chrono::duration<float> elapsed) {
    const float DT = std::max(elapsed.count(), 0.F);
    if (DT <= 0.F)
        return;

    const float MASS      = std::max(spring.mass, 0.0001F);
    const float STIFFNESS = std::max(spring.stiffness, 0.0001F);
    const float DAMPING   = std::max(spring.damping, 0.F);

    const float DISPLACEMENT = value - 1.F;
    const float OMEGA0       = std::sqrt(STIFFNESS / MASS);
    const float GAMMA        = DAMPING / (2.F * MASS);

    if (GAMMA < OMEGA0) {
        const float OMEGAD = std::sqrt((OMEGA0 * OMEGA0) - (GAMMA * GAMMA));
        const float EXP    = std::exp(-GAMMA * DT);
        const float SIN    = std::sin(OMEGAD * DT);
        const float COS    = std::cos(OMEGAD * DT);

        const float NEW_DISPLACEMENT = EXP * ((DISPLACEMENT * COS) + (((velocity + (GAMMA * DISPLACEMENT)) / OMEGAD) * SIN));
        const float NEW_VELOCITY     = EXP * ((velocity * COS) - (((GAMMA * velocity) + (OMEGA0 * OMEGA0 * DISPLACEMENT)) / OMEGAD) * SIN);

        value    = 1.F + NEW_DISPLACEMENT;
        velocity = NEW_VELOCITY;
        return;
    }

    const float CRITICAL_EPSILON = std::max(OMEGA0, 1.F) * 0.0001F;
    if (std::abs(GAMMA - OMEGA0) <= CRITICAL_EPSILON) {
        const float EXP = std::exp(-GAMMA * DT);
        const float B   = velocity + (GAMMA * DISPLACEMENT);

        value    = 1.F + (EXP * (DISPLACEMENT + (B * DT)));
        velocity = EXP * (velocity - (GAMMA * B * DT));
        return;
    }

    const float ROOT = std::sqrt((GAMMA * GAMMA) - (OMEGA0 * OMEGA0));
    const float R1   = -GAMMA + ROOT;
    const float R2   = -GAMMA - ROOT;
    const float A    = (velocity - (R2 * DISPLACEMENT)) / (R1 - R2);
    const float B    = DISPLACEMENT - A;
    const float E1   = std::exp(R1 * DT);
    const float E2   = std::exp(R2 * DT);

    value    = 1.F + (A * E1) + (B * E2);
    velocity = (A * R1 * E1) + (B * R2 * E2);
}
