#include <hyprutils/math/Vector2D.hpp>

#include <gtest/gtest.h>

using namespace Hyprutils::Math;

TEST(Math, vector2d) {
    Vector2D original(30, 40);
    Vector2D monitorSize(100, 200);

    EXPECT_EQ(original.transform(HYPRUTILS_TRANSFORM_NORMAL, monitorSize), Vector2D(30, 40));
    EXPECT_EQ(original.transform(HYPRUTILS_TRANSFORM_90, monitorSize), Vector2D(40, 200 - 30));
    EXPECT_EQ(original.transform(HYPRUTILS_TRANSFORM_180, monitorSize), Vector2D(100 - 30, 200 - 40));
    EXPECT_EQ(original.transform(HYPRUTILS_TRANSFORM_270, monitorSize), Vector2D(100 - 40, 30));
    EXPECT_EQ(original.transform(HYPRUTILS_TRANSFORM_FLIPPED, monitorSize), Vector2D(100 - 30, 40));
    EXPECT_EQ(original.transform(HYPRUTILS_TRANSFORM_FLIPPED_90, monitorSize), Vector2D(40, 30));
    EXPECT_EQ(original.transform(HYPRUTILS_TRANSFORM_FLIPPED_180, monitorSize), Vector2D(30, 200 - 40));
    EXPECT_EQ(original.transform(HYPRUTILS_TRANSFORM_FLIPPED_270, monitorSize), Vector2D(100 - 40, 200 - 30));
}