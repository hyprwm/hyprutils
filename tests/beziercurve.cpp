#include <cmath>
#include <hyprutils/animation/BezierCurve.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include "shared.hpp"

using Hyprutils::Animation::CBezierCurve;
using Hyprutils::Math::Vector2D;

static void test_nonmonotonic4_clamps_out_of_range(int& ret) {
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
    EXPECT(std::isfinite(curve.getYForPoint(0.6f)), true);
    // Far beyond range
    EXPECT(std::isfinite(curve.getYForPoint(std::numeric_limits<float>::max())), true);
    EXPECT(std::isfinite(curve.getYForPoint(-std::numeric_limits<float>::max())), true);
}

static void test_adjacent_baked_x_equal(int& ret) {
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

    EXPECT(y_at_end, 1.0f);
    EXPECT(y_past_end, y_at_end);
}

static void test_all_baked_x_equal(int& ret) {
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

    EXPECT(std::isfinite(y_lo), true);
    EXPECT(std::isfinite(y_0), true);
    EXPECT(std::isfinite(y_hi), true);

    // For this curve Y should stay within [0,1]
    EXPECT((y_lo >= 0.0f && y_lo <= 1.0f), true);
    EXPECT((y_0 >= 0.0f && y_0 <= 1.0f), true);
    EXPECT((y_hi >= 0.0f && y_hi <= 1.0f), true);
}

int main() {
    int ret = 0;

    test_nonmonotonic4_clamps_out_of_range(ret);
    test_adjacent_baked_x_equal(ret);
    test_all_baked_x_equal(ret);

    return ret;
}
