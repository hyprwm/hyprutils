#include <hyprutils/animation/BezierCurve.hpp>
#include <hyprutils/memory/Casts.hpp>

#include <array>
#include <cmath>

using namespace Hyprutils::Animation;
using namespace Hyprutils::Math;
using namespace Hyprutils::Memory;

void CBezierCurve::setup(const std::array<Vector2D, 2>& pVec) {
    setup4(std::array<Vector2D, 4>{
        Vector2D(0, 0),   // Start point
        pVec[0], pVec[1], // Control points
        Vector2D(1, 1)    // End point
    });
}

void CBezierCurve::setup4(const std::array<Vector2D, 4>& pVec) {
    // Avoid reallocations by reserving enough memory upfront
    m_vPoints.resize(4);
    m_vPoints = {
        pVec[0],
        pVec[1],
        pVec[2],
        pVec[3],
    };

    // Pre-bake curve
    //
    // We start baking at t=(i+1)/n not at t=0
    // That means the first baked x can be > 0 if curve itself starts at x>0
    for (int i = 0; i < BAKEDPOINTS; ++i) {
        // When i=0 -> t=1/255
        const float t     = (i + 1) * INVBAKEDPOINTS;
        m_aPointsBaked[i] = Vector2D(getXForT(t), getYForT(t));
    }
}

float CBezierCurve::getXForT(float const& t) const {
    float t2 = t * t;
    float t3 = t2 * t;

    return ((1 - t) * (1 - t) * (1 - t) * m_vPoints[0].x) + (3 * t * (1 - t) * (1 - t) * m_vPoints[1].x) + (3 * t2 * (1 - t) * m_vPoints[2].x) + (t3 * m_vPoints[3].x);
}

float CBezierCurve::getYForT(float const& t) const {
    float t2 = t * t;
    float t3 = t2 * t;

    return ((1 - t) * (1 - t) * (1 - t) * m_vPoints[0].y) + (3 * t * (1 - t) * (1 - t) * m_vPoints[1].y) + (3 * t2 * (1 - t) * m_vPoints[2].y) + (t3 * m_vPoints[3].y);
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

        // Clamp to avoid index walking off
        if (index < 0)
            index = 0;
        else if (index > BAKEDPOINTS - 1)
            index = BAKEDPOINTS - 1;

        below = m_aPointsBaked[index].x < x;
    }

    int lowerIndex = index - (!below || index == BAKEDPOINTS - 1);

    // Clamp final indices
    if (lowerIndex < 0)
        lowerIndex = 0;
    else if (lowerIndex > BAKEDPOINTS - 2)
        lowerIndex = BAKEDPOINTS - 2;

    // In the name of performance I shall make a hack
    const auto& LOWERPOINT = m_aPointsBaked[lowerIndex];
    const auto& UPPERPOINT = m_aPointsBaked[lowerIndex + 1];

    const float dx = (UPPERPOINT.x - LOWERPOINT.x);
    // If two baked points have almost the same x
    //  just return the lower one
    if (dx <= 1e-6f)
        return LOWERPOINT.y;

    const auto PERCINDELTA = (x - LOWERPOINT.x) / dx;

    // Can sometimes happen for VERY small x
    if (std::isnan(PERCINDELTA) || std::isinf(PERCINDELTA))
        return LOWERPOINT.y;

    return LOWERPOINT.y + ((UPPERPOINT.y - LOWERPOINT.y) * PERCINDELTA);
}

const std::vector<Hyprutils::Math::Vector2D>& CBezierCurve::getControlPoints() const {
    return m_vPoints;
}

#ifdef HU_UNIT_TESTS

#include <gtest/gtest.h>

using Hyprutils::Animation::CBezierCurve;
using Hyprutils::Math::Vector2D;

static void test_nonmonotonic4_clamps_out_of_range() {
    // Non-monotonic curve in X
    // This used to drive the step-halving search to OOB. It should now clamp
    CBezierCurve            curve;
    std::array<Vector2D, 4> pts = {
        Vector2D{0.5f, 1.0f}, // P0
        Vector2D{1.0f, 1.0f}, // P1
        Vector2D{0.0f, 0.0f}, // P2
        Vector2D{0.5f, 0.0f}  // P3
    };
    curve.setup4(pts);

    // x > last baked x
    EXPECT_EQ(std::isfinite(curve.getYForPoint(0.6f)), true);
    // Far beyond range
    EXPECT_EQ(std::isfinite(curve.getYForPoint(std::numeric_limits<float>::max())), true);
    EXPECT_EQ(std::isfinite(curve.getYForPoint(-std::numeric_limits<float>::max())), true);
}

static void test_adjacent_baked_x_equal() {
    // Curve with flat tail (X=1, Y=1)
    CBezierCurve            curve;
    std::array<Vector2D, 4> pts = {
        Vector2D{0.0f, 0.0f}, // P0
        Vector2D{0.2f, 0.2f}, // P1
        Vector2D{1.0f, 1.0f}, // P2
        Vector2D{1.0f, 1.0f}  // P3
    };
    curve.setup4(pts);

    // Exactly at last baked X
    const float y_at_end = curve.getYForPoint(1.0f);
    // Slightly beyond last baked X
    const float y_past_end = curve.getYForPoint(1.0001f);

    EXPECT_EQ(y_at_end, 1.0f);
    EXPECT_EQ(y_past_end, y_at_end);
}

static void test_all_baked_x_equal() {
    // Extreme case: X is constant along the whole curve
    CBezierCurve            curve;
    std::array<Vector2D, 4> pts = {
        Vector2D{0.0f, 0.0f}, // P0
        Vector2D{0.0f, 0.3f}, // P1
        Vector2D{0.0f, 0.7f}, // P2
        Vector2D{0.0f, 1.0f}  // P3
    };
    curve.setup4(pts);

    // Below any baked X
    const float y_lo = curve.getYForPoint(-100.0f);
    const float y_0  = curve.getYForPoint(0.0f);
    // Above any baked X
    const float y_hi = curve.getYForPoint(100.0f);

    EXPECT_EQ(std::isfinite(y_lo), true);
    EXPECT_EQ(std::isfinite(y_0), true);
    EXPECT_EQ(std::isfinite(y_hi), true);

    // For this curve Y should stay within [0,1]
    EXPECT_EQ((y_lo >= 0.0f && y_lo <= 1.0f), true);
    EXPECT_EQ((y_0 >= 0.0f && y_0 <= 1.0f), true);
    EXPECT_EQ((y_hi >= 0.0f && y_hi <= 1.0f), true);
}

TEST(Animation, beziercurve) {
    test_nonmonotonic4_clamps_out_of_range();
    test_adjacent_baked_x_equal();
    test_all_baked_x_equal();
}

#endif
