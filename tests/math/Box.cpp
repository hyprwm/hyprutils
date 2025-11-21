#include <hyprutils/math/Box.hpp>
#include <gtest/gtest.h>

using namespace Hyprutils::Math;

TEST(Math, box) {
    // Test default constructor and accessors
    {
        CBox box1;
        EXPECT_EQ(box1.x, 0);
        EXPECT_EQ(box1.y, 0);
        EXPECT_EQ(box1.width, 0);
        EXPECT_EQ(box1.height, 0);

        // Test parameterized constructor and accessors
        CBox box2(10, 20, 30, 40);
        EXPECT_EQ(box2.x, 10);
        EXPECT_EQ(box2.y, 20);
        EXPECT_EQ(box2.width, 30);
        EXPECT_EQ(box2.height, 40);

        // Test setters and getters
        box2.translate(Vector2D(5, -5));
        EXPECT_EQ(box2.pos(), Vector2D(15, 15));
    }

    //Test Scaling and Transformation
    {
        CBox box(10, 10, 20, 30);

        // Test scaling
        box.scale(2.0);
        EXPECT_EQ(box.size(), Vector2D(40, 60));
        EXPECT_EQ(box.pos(), Vector2D(20, 20));

        // Test scaling from center
        box.scaleFromCenter(0.5);
        EXPECT_EQ(box.size(), Vector2D(20, 30));
        EXPECT_EQ(box.pos(), Vector2D(30, 35));

        // Test transformation
        box.transform(HYPRUTILS_TRANSFORM_90, 100, 200);
        EXPECT_EQ(box.pos(), Vector2D(135, 30));
        EXPECT_EQ(box.size(), Vector2D(30, 20));

        // Test Intersection and Extents
    }

    {
        CBox box1(0, 0, 100, 100);
        CBox box2(50, 50, 100, 100);

        CBox intersection = box1.intersection(box2);
        EXPECT_EQ(intersection.pos(), Vector2D(50, 50));
        EXPECT_EQ(intersection.size(), Vector2D(50, 50));

        SBoxExtents extents = box1.extentsFrom(box2);
        EXPECT_EQ(extents.topLeft, Vector2D(50, 50));
        EXPECT_EQ(extents.bottomRight, Vector2D(-50, -50));
    }

    // Test Boundary Conditions and Special Cases
    {
        CBox box(0, 0, 50, 50);

        EXPECT_EQ(box.empty(), false);

        EXPECT_EQ(box.containsPoint(Vector2D(25, 25)), true);
        EXPECT_EQ(box.containsPoint(Vector2D(60, 60)), false);
        EXPECT_EQ(box.overlaps(CBox(25, 25, 50, 50)), true);
        EXPECT_EQ(box.inside(CBox(0, 0, 100, 100)), false);
    }
}