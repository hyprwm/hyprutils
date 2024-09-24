#include <hyprutils/math/Mat3x3.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <hyprutils/math/Box.hpp>
#include <cmath>
#include <unordered_map>
#include <format>

using namespace Hyprutils::Math;

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
    mat.matrix[3] = y * -t.matrix[3];
    mat.matrix[4] = y * -t.matrix[4];

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

    return mat.multiply(*this);
}

Mat3x3& Mat3x3::transform(eTransform transform) {
    multiply(transforms.at(transform));
    return *this;
}

Mat3x3& Mat3x3::rotate(float rot) {
    multiply(std::array<float, 9>{(float)cos(rot), (float)-sin(rot), 0.0f, (float)sin(rot), (float)cos(rot), 0.0f, 0.0f, 0.0f, 1.0f});
    return *this;
}

Mat3x3& Mat3x3::scale(const Vector2D& scale_) {
    multiply(std::array<float, 9>{(float)scale_.x, 0.0f, 0.0f, 0.0f, (float)scale_.y, 0.0f, 0.0f, 0.0f, 1.0f});
    return *this;
}

Mat3x3& Mat3x3::scale(const float scale_) {
    return scale({scale_, scale_});
}

Mat3x3& Mat3x3::translate(const Vector2D& offset) {
    multiply(std::array<float, 9>{1.0f, 0.0f, (float)offset.x, 0.0f, 1.0f, (float)offset.y, 0.0f, 0.0f, 1.0f});
    return *this;
}

Mat3x3& Mat3x3::transpose() {
    matrix = std::array<float, 9>{matrix[0], matrix[3], matrix[6], matrix[1], matrix[4], matrix[7], matrix[2], matrix[5], matrix[8]};
    return *this;
}

Mat3x3& Mat3x3::multiply(const Mat3x3& other) {
    std::array<float, 9> product;

    product[0] = matrix[0] * other.matrix[0] + matrix[1] * other.matrix[3] + matrix[2] * other.matrix[6];
    product[1] = matrix[0] * other.matrix[1] + matrix[1] * other.matrix[4] + matrix[2] * other.matrix[7];
    product[2] = matrix[0] * other.matrix[2] + matrix[1] * other.matrix[5] + matrix[2] * other.matrix[8];

    product[3] = matrix[3] * other.matrix[0] + matrix[4] * other.matrix[3] + matrix[5] * other.matrix[6];
    product[4] = matrix[3] * other.matrix[1] + matrix[4] * other.matrix[4] + matrix[5] * other.matrix[7];
    product[5] = matrix[3] * other.matrix[2] + matrix[4] * other.matrix[5] + matrix[5] * other.matrix[8];

    product[6] = matrix[6] * other.matrix[0] + matrix[7] * other.matrix[3] + matrix[8] * other.matrix[6];
    product[7] = matrix[6] * other.matrix[1] + matrix[7] * other.matrix[4] + matrix[8] * other.matrix[7];
    product[8] = matrix[6] * other.matrix[2] + matrix[7] * other.matrix[5] + matrix[8] * other.matrix[8];

    matrix = product;
    return *this;
}

std::string Mat3x3::toString() const {
    return std::format("[mat3x3: {}, {}, {}, {}, {}, {}, {}, {}, {}]", matrix.at(0), matrix.at(1), matrix.at(2), matrix.at(3), matrix.at(4), matrix.at(5), matrix.at(6),
                       matrix.at(7), matrix.at(8));
}
