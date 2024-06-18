#pragma once

#include "./Vector2D.hpp"
#include "./Misc.hpp"

namespace Hyprutils {
    namespace Math {
        struct SBoxExtents {
            Vector2D topLeft;
            Vector2D bottomRight;

            //
            SBoxExtents operator*(const double& scale) const {
                return SBoxExtents{topLeft * scale, bottomRight * scale};
            }

            SBoxExtents round() {
                return {topLeft.round(), bottomRight.round()};
            }

            bool operator==(const SBoxExtents& other) const {
                return topLeft == other.topLeft && bottomRight == other.bottomRight;
            }

            void addExtents(const SBoxExtents& other) {
                topLeft     = topLeft.getComponentMax(other.topLeft);
                bottomRight = bottomRight.getComponentMax(other.bottomRight);
            }
        };

        class CBox {
          public:
            CBox(double x_, double y_, double w_, double h_) {
                x = x_;
                y = y_;
                w = w_;
                h = h_;
            }

            CBox() {
                w = 0;
                h = 0;
            }

            CBox(const double d) {
                x = d;
                y = d;
                w = d;
                h = d;
            }

            CBox(const Vector2D& pos, const Vector2D& size) {
                x = pos.x;
                y = pos.y;
                w = size.x;
                h = size.y;
            }

            CBox&       applyFromWlr();
            CBox&       scale(double scale);
            CBox&       scaleFromCenter(double scale);
            CBox&       scale(const Vector2D& scale);
            CBox&       translate(const Vector2D& vec);
            CBox&       round();
            CBox&       transform(const eTransform t, double w, double h);
            CBox&       addExtents(const SBoxExtents& e);
            CBox&       expand(const double& value);
            CBox&       noNegativeSize();

            CBox        copy() const;
            CBox        intersection(const CBox& other) const;
            bool        overlaps(const CBox& other) const;
            bool        inside(const CBox& bound) const;

            SBoxExtents extentsFrom(const CBox&); // this is the big box

            Vector2D    middle() const;
            Vector2D    pos() const;
            Vector2D    size() const;
            Vector2D    closestPoint(const Vector2D& vec) const;

            bool        containsPoint(const Vector2D& vec) const;
            bool        empty() const;

            double      x = 0, y = 0;
            union {
                double w;
                double width;
            };
            union {
                double h;
                double height;
            };

            double rot = 0; /* rad, ccw */

            //
            bool operator==(const CBox& rhs) const {
                return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h;
            }

          private:
            CBox roundInternal();
        };
    }
}