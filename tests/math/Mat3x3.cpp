#include <hyprutils/math/Mat3x3.hpp>
#include <hyprutils/math/Box.hpp>

#include <gtest/gtest.h>

using namespace Hyprutils::Math;

TEST(Math, mat3x3) {
    Mat3x3 jeremy    = Mat3x3::outputProjection({1920, 1080}, HYPRUTILS_TRANSFORM_FLIPPED_90);
    Mat3x3 matrixBox = jeremy.projectBox(CBox{10, 10, 200, 200}, HYPRUTILS_TRANSFORM_NORMAL).translate({100, 100}).scale({1.25F, 1.5F}).transpose();

    Mat3x3 expected = std::array<float, 9>{0, 0.46296296, 0, 0.3125, 0, 0, 19.84375, 36.055557, 1};
    // we need to do this to avoid precision errors on 32-bit archs
    EXPECT_EQ(std::abs(expected.getMatrix().at(0) - matrixBox.getMatrix().at(0)) < 0.1, true);
    EXPECT_EQ(std::abs(expected.getMatrix().at(1) - matrixBox.getMatrix().at(1)) < 0.1, true);
    EXPECT_EQ(std::abs(expected.getMatrix().at(2) - matrixBox.getMatrix().at(2)) < 0.1, true);
    EXPECT_EQ(std::abs(expected.getMatrix().at(3) - matrixBox.getMatrix().at(3)) < 0.1, true);
    EXPECT_EQ(std::abs(expected.getMatrix().at(4) - matrixBox.getMatrix().at(4)) < 0.1, true);
    EXPECT_EQ(std::abs(expected.getMatrix().at(5) - matrixBox.getMatrix().at(5)) < 0.1, true);
    EXPECT_EQ(std::abs(expected.getMatrix().at(6) - matrixBox.getMatrix().at(6)) < 0.1, true);
    EXPECT_EQ(std::abs(expected.getMatrix().at(7) - matrixBox.getMatrix().at(7)) < 0.1, true);
    EXPECT_EQ(std::abs(expected.getMatrix().at(8) - matrixBox.getMatrix().at(8)) < 0.1, true);
}