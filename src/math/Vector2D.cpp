#include <hyprutils/math/Vector2D.hpp>
#include <hyprutils/memory/Casts.hpp>
#include <hyprutils/math/Misc.hpp>
#include <algorithm>
#include <cmath>

using namespace Hyprutils::Math;
using namespace Hyprutils::Memory;

Hyprutils::Math::Vector2D::Vector2D(double xx, double yy) : x(xx), y(yy) {
    ;
}

Hyprutils::Math::Vector2D::Vector2D(int xx, int yy) : x(sc<double>(xx)), y(sc<double>(yy)) {
    ;
}

Hyprutils::Math::Vector2D::Vector2D() : x(0), y(0) {
    ;
}

Hyprutils::Math::Vector2D::~Vector2D() {}

double Hyprutils::Math::Vector2D::normalize() {
    // get max abs
    const auto max = std::abs(x) > std::abs(y) ? std::abs(x) : std::abs(y);

    x /= max;
    y /= max;

    return max;
}

Vector2D Hyprutils::Math::Vector2D::floor() const {
    return Vector2D(std::floor(x), std::floor(y));
}

Vector2D Hyprutils::Math::Vector2D::round() const {
    return Vector2D(std::round(x), std::round(y));
}

Vector2D Hyprutils::Math::Vector2D::clamp(const Vector2D& min, const Vector2D& max) const {
    return Vector2D(std::clamp(this->x, min.x, max.x < min.x ? INFINITY : max.x), std::clamp(this->y, min.y, max.y < min.y ? INFINITY : max.y));
}

double Hyprutils::Math::Vector2D::distance(const Vector2D& other) const {
    return std::sqrt(distanceSq(other));
}

double Hyprutils::Math::Vector2D::distanceSq(const Vector2D& other) const {
    return ((x - other.x) * (x - other.x)) + ((y - other.y) * (y - other.y));
}

double Hyprutils::Math::Vector2D::size() const {
    return std::sqrt((x * x) + (y * y));
}

Vector2D Hyprutils::Math::Vector2D::getComponentMax(const Vector2D& other) const {
    return Vector2D(std::max(this->x, other.x), std::max(this->y, other.y));
}

Vector2D Hyprutils::Math::Vector2D::transform(eTransform transform, const Vector2D& monitorSize) const {
    switch (transform) {
        case HYPRUTILS_TRANSFORM_NORMAL: return *this;
        case HYPRUTILS_TRANSFORM_90: return Vector2D(y, monitorSize.y - x);
        case HYPRUTILS_TRANSFORM_180: return Vector2D(monitorSize.x - x, monitorSize.y - y);
        case HYPRUTILS_TRANSFORM_270: return Vector2D(monitorSize.x - y, x);
        case HYPRUTILS_TRANSFORM_FLIPPED: return Vector2D(monitorSize.x - x, y);
        case HYPRUTILS_TRANSFORM_FLIPPED_90: return Vector2D(y, x);
        case HYPRUTILS_TRANSFORM_FLIPPED_180: return Vector2D(x, monitorSize.y - y);
        case HYPRUTILS_TRANSFORM_FLIPPED_270: return Vector2D(monitorSize.x - y, monitorSize.y - x);
        default: return *this;
    }
}
