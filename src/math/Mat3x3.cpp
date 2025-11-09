#include <hyprutils/math/Mat3x3.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <hyprutils/math/Box.hpp>
#include <hyprutils/memory/Casts.hpp>
#include <cmath>
#include <unordered_map>
#include <format>

using namespace Hyprutils::Math;
using namespace Hyprutils::Memory;

static std::unordered_map<eTransform, Mat3x3> transforms = {
    {HYPRUTILS_TRANSFORM_NORMAL, std::array<float, 9>{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}},
    {HYPRUTILS_TRANSFORM_90, std::array<float, 9>{0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}},
    {HYPRUTILS_TRANSFORM_180, std::array<float, 9>{-1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f}},
    {HYPRUTILS_TRANSFORM_270, std::array<float, 9>{0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}},
    {HYPRUTILS_TRANSFORM_FLIPPED, std::array<float, 9>{-1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}},
    {HYPRUTILS_TRANSFORM_FLIPPED_90, std::array<float, 9>{0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}},
    {HYPRUTILS_TRANSFORM_FLIPPED_180, std::array<float, 9>{1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f}},
    {HYPRUTILS_TRANSFORM_FLIPPED_270, std::array<float, 9>{0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}},
};

Mat3x3::Mat3x3() {
    matrix = {0};
}

Mat3x3::Mat3x3(std::array<float, 9> mat) : matrix(mat) {
    ;
}

Mat3x3::Mat3x3(std::vector<float> mat) {
    for (size_t i = 0; i < 9; ++i) {
        matrix.at(i) = mat.size() < i ? mat.at(i) : 0.F;
    }
}

Mat3x3 Mat3x3::identity() {
    return Mat3x3(std::array<float, 9>{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});
}

Mat3x3 Mat3x3::outputProjection(const Vector2D& size, eTransform transform) {
    Mat3x3      mat;

    const auto& t = transforms.at(transform);
    float       x = 2.0f / size.x;
    float       y = 2.0f / size.y;

    // Rotation + reflection
    mat.matrix[0] = x * t.matrix[0];
    mat.matrix[1] = x * t.matrix[1];
    mat.matrix[3] = y * t.matrix[3];
    mat.matrix[4] = y * t.matrix[4];

    // Translation
    mat.matrix[2] = -copysign(1.0f, mat.matrix[0] + mat.matrix[1]);
    mat.matrix[5] = -copysign(1.0f, mat.matrix[3] + mat.matrix[4]);

    // Identity
    mat.matrix[8] = 1.0f;

    return mat;
}

std::array<float, 9> Mat3x3::getMatrix() const {
    return matrix;
}

Mat3x3 Mat3x3::projectBox(const CBox& box, eTransform transform, float rot /* rad, CCW */) const {
    Mat3x3     mat = Mat3x3::identity();

    const auto boxSize = box.size();

    mat.translate(box.pos());

    if (rot != 0) {
        mat.translate(boxSize / 2);
        mat.rotate(rot);
        mat.translate(-boxSize / 2);
    }

    mat.scale(boxSize);

    if (transform != HYPRUTILS_TRANSFORM_NORMAL) {
        mat.translate({0.5, 0.5});
        mat.transform(transform);
        mat.translate({-0.5, -0.5});
    }

    return this->copy().multiply(mat);
}

Mat3x3& Mat3x3::transform(eTransform transform) {
    multiply(transforms.at(transform));
    return *this;
}

Mat3x3& Mat3x3::rotate(float rot) {
    multiply(std::array<float, 9>{cosf(rot), -sinf(rot), 0.0f, sinf(rot), cosf(rot), 0.0f, 0.0f, 0.0f, 1.0f});
    return *this;
}

Mat3x3& Mat3x3::scale(const Vector2D& scale_) {
    multiply(std::array<float, 9>{sc<float>(scale_.x), 0.0f, 0.0f, 0.0f, sc<float>(scale_.y), 0.0f, 0.0f, 0.0f, 1.0f});
    return *this;
}

Mat3x3& Mat3x3::scale(const float scale_) {
    return scale({scale_, scale_});
}

Mat3x3& Mat3x3::translate(const Vector2D& offset) {
    multiply(std::array<float, 9>{1.0f, 0.0f, sc<float>(offset.x), 0.0f, 1.0f, sc<float>(offset.y), 0.0f, 0.0f, 1.0f});
    return *this;
}

Mat3x3& Mat3x3::transpose() {
    matrix = std::array<float, 9>{matrix[0], matrix[3], matrix[6], matrix[1], matrix[4], matrix[7], matrix[2], matrix[5], matrix[8]};
    return *this;
}

Mat3x3& Mat3x3::multiply(const Mat3x3& other) {
    const float*         m1 = matrix.data();       // Pointer to current matrix
    const float*         m2 = other.matrix.data(); // Pointer to the other matrix

    std::array<float, 9> product;

    product[0] = m1[0] * m2[0] + m1[1] * m2[3] + m1[2] * m2[6];
    product[1] = m1[0] * m2[1] + m1[1] * m2[4] + m1[2] * m2[7];
    product[2] = m1[0] * m2[2] + m1[1] * m2[5] + m1[2] * m2[8];

    product[3] = m1[3] * m2[0] + m1[4] * m2[3] + m1[5] * m2[6];
    product[4] = m1[3] * m2[1] + m1[4] * m2[4] + m1[5] * m2[7];
    product[5] = m1[3] * m2[2] + m1[4] * m2[5] + m1[5] * m2[8];

    product[6] = m1[6] * m2[0] + m1[7] * m2[3] + m1[8] * m2[6];
    product[7] = m1[6] * m2[1] + m1[7] * m2[4] + m1[8] * m2[7];
    product[8] = m1[6] * m2[2] + m1[7] * m2[5] + m1[8] * m2[8];

    matrix = product;
    return *this;
}

Mat3x3 Mat3x3::copy() const {
    return *this;
}

std::string Mat3x3::toString() const {
    for (const auto& m : matrix) {
        if (!std::isfinite(m))
            return "[mat3x3: invalid values]";
    }

    return std::format("[mat3x3: {}, {}, {}, {}, {}, {}, {}, {}, {}]", matrix.at(0), matrix.at(1), matrix.at(2), matrix.at(3), matrix.at(4), matrix.at(5), matrix.at(6),
                       matrix.at(7), matrix.at(8));
}

#ifdef HU_UNIT_TESTS

#include <gtest/gtest.h>

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

#endif
