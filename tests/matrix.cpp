#include <hyprutils/math/Matrix.hpp>
#include "shared.hpp"
#include <cmath>

using namespace Hyprutils::Math;

int main(int argc, char** argv, char** envp) {
    int ret = 0;

    {
        auto  testName       = "matrixTranslate";
        float mat[9]         = {1, 0, 0, 0, 1, 0, 0, 0, 1};
        float x              = 5.0f;
        float y              = -3.0f;
        float expectedMat[9] = {1.0f, 0.0f, 5.0f, 0.0f, 1.0f, -3.0f, 0.0f, 0.0f, 1.0f};
        matrixTranslate(mat, x, y);
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixMultiply";
        float a[9]     = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        float b[9]     = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
        float resultMat[9];
        float expectedMat[9] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
        matrixMultiply(resultMat, a, b);
        EXPECT_MATRIX(resultMat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixIdentity";
        float mat[9];
        float expectedMat[9] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        matrixIdentity(mat);
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName       = "matrixScale";
        float mat[9]         = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        float expectedMat[9] = {2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        matrixScale(mat, 2.0f, 3.0f);
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixTranspose";
        float mat[9];
        float inputMat[9]    = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
        float expectedMat[9] = {1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f};
        matrixTranspose(mat, inputMat);
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName       = "matrixRotate90";
        float mat[9]         = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        float angle          = M_PI_2; // 90 degrees in radians
        float expectedMat[9] = {0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        matrixRotate(mat, angle);
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName       = "matrixRotate_180";
        float mat[9]         = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        float angle          = M_PI; // 180 degrees in radians
        float expectedMat[9] = {-1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        matrixRotate(mat, angle);
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName       = "matrixRotate_360";
        float mat[9]         = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        float angle          = M_2_PI; // 360 degrees in radians
        float expectedMat[9] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        matrixRotate(mat, angle);
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixTransformFlipped";
        float mat[9]   = {
            1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        matrixTransform(mat, HYPRUTILS_TRANSFORM_FLIPPED);
        float expectedMat[9] = {
            -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixTransform90";
        float mat[9]   = {
            1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        matrixTransform(mat, HYPRUTILS_TRANSFORM_90);
        float expectedMat[9] = {
            0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixTransform";
        float mat[9]   = {
            1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        matrixTransform(mat, HYPRUTILS_TRANSFORM_180);
        float expectedMat[9] = {
            -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixProjection_NORMAL";
        int   width    = 640;
        int   height   = 480;
        float mat[9];
        matrixProjection(mat, width, height, HYPRUTILS_TRANSFORM_NORMAL);
        float xScale         = 2.0f / width;
        float yScale         = 2.0f / height;
        float expectedMat[9] = {xScale, 0.0f, -1.0f, 0.0f, -yScale, 1.0f, 0.0f, 0.0f, 1.0f};
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixProjection_90";
        int   width    = 640;
        int   height   = 480;
        float mat[9];
        matrixProjection(mat, width, height, HYPRUTILS_TRANSFORM_90);
        float xScale         = 2.0f / width;
        float yScale         = 2.0f / height;
        float expectedMat[9] = {0.0f, xScale, -1.0f, yScale, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixProjection_180";
        int   width    = 640;
        int   height   = 480;
        float mat[9];
        matrixProjection(mat, width, height, HYPRUTILS_TRANSFORM_180);
        float xScale         = 2.0f / width;
        float yScale         = 2.0f / height;
        float expectedMat[9] = {-xScale, 0.0f, 1.0f, 0.0f, yScale, -1.0f, 0.0f, 0.0f, 1.0f};
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto  testName = "matrixProjection_270";
        int   width    = 640;
        int   height   = 480;
        float mat[9];
        matrixProjection(mat, width, height, HYPRUTILS_TRANSFORM_270);
        float xScale         = 2.0f / width;
        float yScale         = 2.0f / height;
        float expectedMat[9] = {0.0f, -xScale, 1.0f, -yScale, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto       testName = "projectBox";
        float      mat[9];
        CBox       box           = {0, 0, 100, 100};
        eTransform transform     = HYPRUTILS_TRANSFORM_NORMAL;
        float      rotation      = 0;
        float      projection[9] = {
            1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        projectBox(mat, box, transform, rotation, projection);
        float expectedMat[9] = {
            100.0f, 0.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto       testName = "projectBox_rotation";
        float      mat[9];
        CBox       box           = {0, 0, 100, 100};
        eTransform transform     = HYPRUTILS_TRANSFORM_NORMAL;
        float      rotation      = M_PI; // 180 degrees
        float      projection[9] = {
            1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        projectBox(mat, box, transform, rotation, projection);
        float expectedMat[9] = {
            -100.0f, 0.0f, 100.0f, 0.0f, -100.0f, 100.0f, 0.0f, 0.0f, 1.0f,
        };
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    {
        auto       testName = "projectBox_transform";
        float      mat[9];
        CBox       box           = {0, 0, 100, 100};
        eTransform transform     = HYPRUTILS_TRANSFORM_90;
        float      rotation      = 0;
        float      projection[9] = {
            1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        projectBox(mat, box, transform, rotation, projection);
        float expectedMat[9] = {
            0.0f, 100.0f, 0.0f, -100.0f, 0.0f, 100.0f, 0.0f, 0.0f, 1.0f,
        };
        EXPECT_MATRIX(mat, expectedMat, 9, testName);
    }

    return ret;
}