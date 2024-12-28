#include <hyprutils/animation/BezierCurve.hpp>

#include <array>
#include <cmath>

using namespace Hyprutils::Animation;
using namespace Hyprutils::Math;

void CBezierCurve::setup(const std::array<Vector2D, 2>& pVec) {
    // Avoid reallocations by reserving enough memory upfront
    m_vPoints.resize(pVec.size() + 2);
    m_vPoints = {
        Vector2D(0, 0),   // Start point
        pVec[0], pVec[1], // Control points
        Vector2D(1, 1)    // End point
    };

    if (m_vPoints.size() != 4)
        std::abort();

    // bake BAKEDPOINTS points for faster lookups
    // T -> X ( / BAKEDPOINTS )
    for (int i = 0; i < BAKEDPOINTS; ++i) {
        float const t     = (i + 1) / (float)BAKEDPOINTS;
        m_aPointsBaked[i] = Vector2D(getXForT(t), getYForT(t));
    }

    for (int j = 1; j < 10; ++j) {
        float i = j / 10.0f;
        getYForPoint(i);
    }
}

float CBezierCurve::getXForT(float const& t) const {
    float t2 = t * t;
    float t3 = t2 * t;

    return 3 * t * (1 - t) * (1 - t) * m_vPoints[1].x + 3 * t2 * (1 - t) * m_vPoints[2].x + t3 * m_vPoints[3].x;
}

float CBezierCurve::getYForT(float const& t) const {
    float t2 = t * t;
    float t3 = t2 * t;

    return 3 * t * (1 - t) * (1 - t) * m_vPoints[1].y + 3 * t2 * (1 - t) * m_vPoints[2].y + t3 * m_vPoints[3].y;
}

// Todo: this probably can be done better and faster
float CBezierCurve::getYForPoint(float const& x) const {
    if (x >= 1.f)
        return 1.f;
    if (x <= 0.f)
        return 0.f;

    int  index = 0;
    bool below = true;
    for (int step = (BAKEDPOINTS + 1) / 2; step > 0; step /= 2) {
        if (below)
            index += step;
        else
            index -= step;

        below = m_aPointsBaked[index].x < x;
    }

    int lowerIndex = index - (!below || index == BAKEDPOINTS - 1);

    // in the name of performance i shall make a hack
    const auto LOWERPOINT = &m_aPointsBaked[lowerIndex];
    const auto UPPERPOINT = &m_aPointsBaked[lowerIndex + 1];

    const auto PERCINDELTA = (x - LOWERPOINT->x) / (UPPERPOINT->x - LOWERPOINT->x);

    if (std::isnan(PERCINDELTA) || std::isinf(PERCINDELTA)) // can sometimes happen for VERY small x
        return 0.f;

    return LOWERPOINT->y + (UPPERPOINT->y - LOWERPOINT->y) * PERCINDELTA;
}
