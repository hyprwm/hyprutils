#pragma once

#include <array>
#include <vector>
#include <string>
#include <ostream>

#include "./Misc.hpp"

namespace Hyprutils {
    namespace Math {
        class CBox;
        class Vector2D;

        class Mat3x3 {
          public:
            Mat3x3();
            Mat3x3(std::array<float, 9>);
            Mat3x3(std::vector<float>);

            /* create an identity 3x3 matrix */
            static Mat3x3 identity();

            /* create an output projection matrix */
            static Mat3x3 outputProjection(const Vector2D& size, eTransform transform);

            /* get the matrix as an array, in a row-major order. */
            std::array<float, 9> getMatrix() const;

            /* create a box projection matrix */
            Mat3x3 projectBox(const CBox& box, eTransform transform, float rot = 0.F /* rad, CCW */) const;

            /* in-place functions */
            Mat3x3& transform(eTransform transform);
            Mat3x3& rotate(float rot /* rad, CCW */);
            Mat3x3& scale(const Vector2D& scale);
            Mat3x3& scale(const float scale);
            Mat3x3& translate(const Vector2D& offset);
            Mat3x3& transpose();
            Mat3x3& multiply(const Mat3x3& other);

            /* misc utils */
            Mat3x3      copy() const;
            std::string toString() const;

            bool        operator==(const Mat3x3& other) const {
                return other.matrix == matrix;
            }

            friend std::ostream& operator<<(std::ostream& os, const Mat3x3& mat) {
                os << mat.toString();
                return os;
            }

          private:
            std::array<float, 9> matrix;
        };
    }
}