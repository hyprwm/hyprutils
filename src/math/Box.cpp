#include <hyprutils/math/Box.hpp>
#include <limits>
#include <algorithm>
#include <cmath>

#define VECINRECT(vec, x1, y1, x2, y2) ((vec).x >= (x1) && (vec).x < (x2) && (vec).y >= (y1) && (vec).y < (y2))

using namespace Hyprutils::Math;

CBox& Hyprutils::Math::CBox::scale(double scale) {
    x *= scale;
    y *= scale;
    w *= scale;
    h *= scale;

    return *this;
}

CBox& Hyprutils::Math::CBox::scale(const Vector2D& scale) {
    x *= scale.x;
    y *= scale.y;
    w *= scale.x;
    h *= scale.y;

    return *this;
}

CBox& Hyprutils::Math::CBox::translate(const Vector2D& vec) {
    x += vec.x;
    y += vec.y;

    return *this;
}

Vector2D Hyprutils::Math::CBox::middle() const {
    return Vector2D{x + w / 2.0, y + h / 2.0};
}

bool Hyprutils::Math::CBox::containsPoint(const Vector2D& vec) const {
    return VECINRECT(vec, x, y, x + w, y + h);
}

bool Hyprutils::Math::CBox::empty() const {
    return w == 0 || h == 0;
}

CBox& Hyprutils::Math::CBox::round() {
    float newW = x + w - std::round(x);
    float newH = y + h - std::round(y);
    x          = std::round(x);
    y          = std::round(y);
    w          = std::round(newW);
    h          = std::round(newH);

    return *this;
}

CBox& Hyprutils::Math::CBox::transform(const eTransform t, double w, double h) {
    CBox temp = *this;

    if (t % 2 == 0) {
        width  = temp.width;
        height = temp.height;
    } else {
        width  = temp.height;
        height = temp.width;
    }

    switch (t) {
        case HYPRUTILS_TRANSFORM_NORMAL:
            x = temp.x;
            y = temp.y;
            break;
        case HYPRUTILS_TRANSFORM_90:
            x = h - temp.y - temp.height;
            y = temp.x;
            break;
        case HYPRUTILS_TRANSFORM_180:
            x = w - temp.x - temp.width;
            y = h - temp.y - temp.height;
            break;
        case HYPRUTILS_TRANSFORM_270:
            x = temp.y;
            y = w - temp.x - temp.width;
            break;
        case HYPRUTILS_TRANSFORM_FLIPPED:
            x = w - temp.x - temp.width;
            y = temp.y;
            break;
        case HYPRUTILS_TRANSFORM_FLIPPED_90:
            x = temp.y;
            y = temp.x;
            break;
        case HYPRUTILS_TRANSFORM_FLIPPED_180:
            x = temp.x;
            y = h - temp.y - temp.height;
            break;
        case HYPRUTILS_TRANSFORM_FLIPPED_270:
            x = h - temp.y - temp.height;
            y = w - temp.x - temp.width;
            break;
    }

    return *this;
}

CBox& Hyprutils::Math::CBox::addExtents(const SBoxExtents& e) {
    x -= e.topLeft.x;
    y -= e.topLeft.y;
    w += e.topLeft.x + e.bottomRight.x;
    h += e.topLeft.y + e.bottomRight.y;

    return *this;
}

CBox& Hyprutils::Math::CBox::scaleFromCenter(double scale) {
    double oldW = w, oldH = h;

    w *= scale;
    h *= scale;

    x -= (w - oldW) / 2.0;
    y -= (h - oldH) / 2.0;

    return *this;
}

CBox& Hyprutils::Math::CBox::expand(const double& value) {
    x -= value;
    y -= value;
    w += value * 2.0;
    h += value * 2.0;

    if (w <= 0 || h <= 0) {
        w = 0;
        h = 0;
    }

    return *this;
}

CBox& Hyprutils::Math::CBox::noNegativeSize() {
    w = std::clamp(w, 0.0, std::numeric_limits<double>::infinity());
    h = std::clamp(h, 0.0, std::numeric_limits<double>::infinity());

    return *this;
}

CBox Hyprutils::Math::CBox::intersection(const CBox& other) const {
    const float newX      = std::max(x, other.x);
    const float newY      = std::max(y, other.y);
    const float newBottom = std::min(y + h, other.y + other.h);
    const float newRight  = std::min(x + w, other.x + other.w);
    float       newW      = newRight - newX;
    float       newH      = newBottom - newY;

    if (newW <= 0 || newH <= 0) {
        newW = 0;
        newH = 0;
    }

    return {newX, newY, newW, newH};
}

bool Hyprutils::Math::CBox::overlaps(const CBox& other) const {
    return (other.x + other.w >= x) && (x + w >= other.x) && (other.y + other.h >= y) && (y + h >= other.y);
}

bool Hyprutils::Math::CBox::inside(const CBox& bound) const {
    return bound.x < x && bound.y < y && x + w < bound.x + bound.w && y + h < bound.y + bound.h;
}

CBox Hyprutils::Math::CBox::roundInternal() {
    float newW = x + w - std::floor(x);
    float newH = y + h - std::floor(y);

    return CBox{std::floor(x), std::floor(y), std::floor(newW), std::floor(newH)};
}

CBox Hyprutils::Math::CBox::copy() const {
    return CBox{*this};
}

Vector2D Hyprutils::Math::CBox::pos() const {
    return {x, y};
}

Vector2D Hyprutils::Math::CBox::size() const {
    return {w, h};
}

Vector2D Hyprutils::Math::CBox::closestPoint(const Vector2D& vec) const {
    if (containsPoint(vec))
        return vec;

    Vector2D nv = vec;
    nv.x        = std::clamp(nv.x, x, x + w);
    nv.y        = std::clamp(nv.y, y, y + h);
    return nv;
}

SBoxExtents Hyprutils::Math::CBox::extentsFrom(const CBox& small) {
    return {{small.x - x, small.y - y}, {w - small.w - (small.x - x), h - small.h - (small.y - y)}};
}
