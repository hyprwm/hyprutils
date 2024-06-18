#include <hyprutils/math/Vector2D.hpp>
#include <algorithm>
#include <cmath>

using namespace Hyprutils::Math;

Hyprutils::Math::Vector2D::Vector2D(double xx, double yy) {
    x = xx;
    y = yy;
}

Hyprutils::Math::Vector2D::Vector2D() {
    x = 0;
    y = 0;
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
    return (x - other.x) * (x - other.x) + (y - other.y) * (y - other.y);
}

double Hyprutils::Math::Vector2D::size() const {
    return std::sqrt(x * x + y * y);
}

Vector2D Hyprutils::Math::Vector2D::getComponentMax(const Vector2D& other) const {
    return Vector2D(std::max(this->x, other.x), std::max(this->y, other.y));
}
