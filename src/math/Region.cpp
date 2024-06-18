#include <hyprutils/math/Region.hpp>
#include <cmath>

using namespace Hyprutils::Math;

constexpr const int64_t MAX_REGION_SIDE = 10000000;

Hyprutils::Math::CRegion::CRegion() {
    pixman_region32_init(&m_rRegion);
}

Hyprutils::Math::CRegion::CRegion(const pixman_region32_t* const ref) {
    pixman_region32_init(&m_rRegion);
    pixman_region32_copy(&m_rRegion, ref);
}

Hyprutils::Math::CRegion::CRegion(double x, double y, double w, double h) {
    pixman_region32_init_rect(&m_rRegion, x, y, w, h);
}

Hyprutils::Math::CRegion::CRegion(const CBox& box) {
    pixman_region32_init_rect(&m_rRegion, box.x, box.y, box.w, box.h);
}

Hyprutils::Math::CRegion::CRegion(pixman_box32_t* box) {
    pixman_region32_init_rect(&m_rRegion, box->x1, box->y1, box->x2 - box->x1, box->y2 - box->y1);
}

Hyprutils::Math::CRegion::CRegion(const CRegion& other) {
    pixman_region32_init(&m_rRegion);
    pixman_region32_copy(&m_rRegion, const_cast<CRegion*>(&other)->pixman());
}

Hyprutils::Math::CRegion::CRegion(CRegion&& other) {
    pixman_region32_init(&m_rRegion);
    pixman_region32_copy(&m_rRegion, other.pixman());
}

Hyprutils::Math::CRegion::~CRegion() {
    pixman_region32_fini(&m_rRegion);
}

CRegion& Hyprutils::Math::CRegion::clear() {
    pixman_region32_clear(&m_rRegion);
    return *this;
}

CRegion& Hyprutils::Math::CRegion::set(const CRegion& other) {
    pixman_region32_copy(&m_rRegion, const_cast<CRegion*>(&other)->pixman());
    return *this;
}

CRegion& Hyprutils::Math::CRegion::add(const CRegion& other) {
    pixman_region32_union(&m_rRegion, &m_rRegion, const_cast<CRegion*>(&other)->pixman());
    return *this;
}

CRegion& Hyprutils::Math::CRegion::add(double x, double y, double w, double h) {
    pixman_region32_union_rect(&m_rRegion, &m_rRegion, x, y, w, h);
    return *this;
}

CRegion& Hyprutils::Math::CRegion::add(const CBox& other) {
    pixman_region32_union_rect(&m_rRegion, &m_rRegion, other.x, other.y, other.w, other.h);
    return *this;
}

CRegion& Hyprutils::Math::CRegion::subtract(const CRegion& other) {
    pixman_region32_subtract(&m_rRegion, &m_rRegion, const_cast<CRegion*>(&other)->pixman());
    return *this;
}

CRegion& Hyprutils::Math::CRegion::intersect(const CRegion& other) {
    pixman_region32_intersect(&m_rRegion, &m_rRegion, const_cast<CRegion*>(&other)->pixman());
    return *this;
}

CRegion& Hyprutils::Math::CRegion::intersect(double x, double y, double w, double h) {
    pixman_region32_intersect_rect(&m_rRegion, &m_rRegion, x, y, w, h);
    return *this;
}

CRegion& Hyprutils::Math::CRegion::invert(pixman_box32_t* box) {
    pixman_region32_inverse(&m_rRegion, &m_rRegion, box);
    return *this;
}

CRegion& Hyprutils::Math::CRegion::invert(const CBox& box) {
    pixman_box32 pixmanBox = {(int32_t)box.x, (int32_t)box.y, (int32_t)box.w + (int32_t)box.x, (int32_t)box.h + (int32_t)box.y};
    return this->invert(&pixmanBox);
}

CRegion& Hyprutils::Math::CRegion::translate(const Vector2D& vec) {
    pixman_region32_translate(&m_rRegion, vec.x, vec.y);
    return *this;
}

CRegion& Hyprutils::Math::CRegion::transform(const eTransform t, double w, double h) {
    if (t == HYPRUTILS_TRANSFORM_NORMAL)
        return *this;

    auto rects = getRects();

    clear();

    for (auto& r : rects) {
        CBox xfmd{(double)r.x1, (double)r.y1, (double)r.x2 - r.x1, (double)r.y2 - r.y1};
        xfmd.transform(t, w, h);
        add(xfmd);
    }

    return *this;
}

CRegion& Hyprutils::Math::CRegion::rationalize() {
    intersect(CBox{-MAX_REGION_SIDE, -MAX_REGION_SIDE, MAX_REGION_SIDE * 2, MAX_REGION_SIDE * 2});
    return *this;
}

CRegion Hyprutils::Math::CRegion::copy() const {
    return CRegion(*this);
}

CRegion& Hyprutils::Math::CRegion::scale(float scale_) {
    scale({scale_, scale_});
    return *this;
}

CRegion& Hyprutils::Math::CRegion::scale(const Vector2D& scale) {
    if (scale == Vector2D{1, 1})
        return *this;

    auto rects = getRects();

    clear();

    for (auto& r : rects) {
        r.x1 = std::floor(r.x1 * scale.x);
        r.y1 = std::floor(r.y1 * scale.x);
        r.x2 = std::ceil(r.x2 * scale.x);
        r.y2 = std::ceil(r.y2 * scale.x);
        add(&r);
    }

    return *this;
}

std::vector<pixman_box32_t> Hyprutils::Math::CRegion::getRects() const {
    std::vector<pixman_box32_t> result;

    int                         rectsNum = 0;
    const auto                  RECTSARR = pixman_region32_rectangles(&m_rRegion, &rectsNum);

    result.assign(RECTSARR, RECTSARR + rectsNum);

    return result;
}

CBox Hyprutils::Math::CRegion::getExtents() {
    pixman_box32_t* box = pixman_region32_extents(&m_rRegion);
    return {(double)box->x1, (double)box->y1, (double)box->x2 - box->x1, (double)box->y2 - box->y1};
}

bool Hyprutils::Math::CRegion::containsPoint(const Vector2D& vec) const {
    return pixman_region32_contains_point(&m_rRegion, vec.x, vec.y, nullptr);
}

bool Hyprutils::Math::CRegion::empty() const {
    return !pixman_region32_not_empty(&m_rRegion);
}

Vector2D Hyprutils::Math::CRegion::closestPoint(const Vector2D& vec) const {
    if (containsPoint(vec))
        return vec;

    double   bestDist = __FLT_MAX__;
    Vector2D leader   = vec;

    for (auto& box : getRects()) {
        double x = 0, y = 0;

        if (vec.x >= box.x2)
            x = box.x2 - 1;
        else if (vec.x < box.x1)
            x = box.x1;
        else
            x = vec.x;

        if (vec.y >= box.y2)
            y = box.y2 - 1;
        else if (vec.y < box.y1)
            y = box.y1;
        else
            y = vec.y;

        double distance = pow(x, 2) + pow(y, 2);
        if (distance < bestDist) {
            bestDist = distance;
            leader   = {x, y};
        }
    }

    return leader;
}