#pragma once

#include "./Vector2D.hpp"
#include "./Misc.hpp"

namespace Hyprutils::Math {

    /**
        * @brief Represents the extents of a bounding box.
        */
    struct SBoxExtents {
        Vector2D topLeft;
        Vector2D bottomRight;

        /**
            * @brief Scales the extents by a given factor.
            * @param scale The scaling factor.
            * @return Scaled SBoxExtents.
            */
        SBoxExtents operator*(const double& scale) const {
            return SBoxExtents{topLeft * scale, bottomRight * scale};
        }
        /**
            * @brief Rounds the coordinates of the extents.
            * @return Rounded SBoxExtents.
            */
        SBoxExtents round() {
            return {topLeft.round(), bottomRight.round()};
        }
        /**
            * @brief Checks equality between two SBoxExtents objects.
            * @param other Another SBoxExtents object to compare.
            * @return True if both SBoxExtents are equal, false otherwise.
            */
        bool operator==(const SBoxExtents& other) const {
            return topLeft == other.topLeft && bottomRight == other.bottomRight;
        }

        /**
            * @brief Adjusts the extents to encompass another SBoxExtents.
            * @param other Another SBoxExtents to add to this one.
            */
        void addExtents(const SBoxExtents& other) {
            topLeft     = topLeft.getComponentMax(other.topLeft);
            bottomRight = bottomRight.getComponentMax(other.bottomRight);
        }
    };

    /**
        * @brief Represents a 2D bounding box.
        */
    class CBox {
      public:
        /**
            * @brief Constructs a CBox with specified position and dimensions.
            * @param x_ X-coordinate of the top-left corner.
            * @param y_ Y-coordinate of the top-left corner.
            * @param w_ Width of the box.
            * @param h_ Height of the box.
            */
        CBox(double x_, double y_, double w_, double h_) {
            x = x_;
            y = y_;
            w = w_;
            h = h_;
        }
        /**
            * @brief Default constructor. Initializes an empty box (0 width, 0 height).
            */
        CBox() {
            w = 0;
            h = 0;
        }
        /**
            * @brief Constructs a CBox with uniform dimensions.
            * @param d Dimensions to apply uniformly (x, y, width, height).
            */
        CBox(const double d) {
            x = d;
            y = d;
            w = d;
            h = d;
        }
        /**
            * @brief Constructs a CBox from a position and size vector.
            * @param pos Position vector representing the top-left corner.
            * @param size Size vector representing width and height.
            */
        CBox(const Vector2D& pos, const Vector2D& size) {
            x = pos.x;
            y = pos.y;
            w = size.x;
            h = size.y;
        }

        // Geometric operations
        CBox& scale(double scale);
        CBox& scaleFromCenter(double scale);
        CBox& scale(const Vector2D& scale);
        CBox& translate(const Vector2D& vec);
        CBox& round();
        CBox& transform(const eTransform t, double w, double h);
        CBox& addExtents(const SBoxExtents& e);
        CBox& expand(const double& value);
        CBox& noNegativeSize();

        CBox  copy() const;
        CBox  intersection(const CBox& other) const;
        bool  overlaps(const CBox& other) const;
        bool  inside(const CBox& bound) const;

        /**
            * @brief Computes the extents of the box relative to another box.
            * @param small Another CBox to compare against.
            * @return SBoxExtents representing the extents of the box relative to 'small'.
            */
        SBoxExtents extentsFrom(const CBox&); // this is the big box

        /**
            * @brief Calculates the middle point of the box.
            * @return Vector2D representing the middle point.
            */
        Vector2D middle() const;

        /**
            * @brief Retrieves the position of the top-left corner of the box.
            * @return Vector2D representing the position.
            */
        Vector2D pos() const;

        /**
            * @brief Retrieves the size (width and height) of the box.
            * @return Vector2D representing the size.
            */
        Vector2D size() const;

        /**
            * @brief Retrieves the size of the box offset by its position.
            * @return Vector2D representing the bottom right extent of the box.
            */
        Vector2D extent() const;

        /**
            * @brief Finds the closest point within the box to a given vector.
            * @param vec Vector from which to find the closest point.
            * @return Vector2D representing the closest point within the box.
            */
        Vector2D closestPoint(const Vector2D& vec) const;

        /**
            * @brief Checks if a given point is inside the box.
            * @param vec Vector representing the point to check.
            * @return True if the point is inside the box, false otherwise.
            */
        bool containsPoint(const Vector2D& vec) const;

        /**
            * @brief Checks if the box is empty (zero width or height).
            * @return True if the box is empty, false otherwise.
            */
        bool   empty() const;

        double x = 0, y = 0; // Position of the top-left corner of the box.
        union {
            double w;
            double width;
        };
        union {
            double h;
            double height;
        };

        double rot = 0; //< Rotation angle of the box in radians (counterclockwise).

        /**
            * @brief Checks equality between two CBox objects.
            * @param rhs Another CBox object to compare.
            * @return True if both CBox objects are equal, false otherwise.
            */
        bool operator==(const CBox& rhs) const {
            return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h;
        }

      private:
        CBox roundInternal();
    };
}
