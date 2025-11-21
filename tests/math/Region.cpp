#include <hyprutils/math/Region.hpp>

#include <gtest/gtest.h>

using namespace Hyprutils::Math;

TEST(Math, region) {
    CRegion rg(CBox{{20, 20}, {40, 40}});

    auto    extents = rg.getExtents();
    EXPECT_EQ(extents.pos(), Vector2D(20, 20));
    EXPECT_EQ(extents.size(), Vector2D(40, 40));

    rg.scale(2);
    extents = rg.getExtents();
    EXPECT_EQ(extents.pos(), Vector2D(40, 40));
    EXPECT_EQ(extents.size(), Vector2D(80, 80));
}