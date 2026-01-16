#pragma once

#include <pixman.h>
#include <vector>
#include "Vector2D.hpp"
#include "Box.hpp"

namespace Hyprutils {
    namespace Math {
        class CRegion {
          public:
            /* Create an empty region */
            CRegion();
            /* Create from a reference. Copies, does not own. */
            CRegion(const pixman_region32_t* const ref);
            /* Create from a box */
            CRegion(double x, double y, double w, double h);
            /* Create from a CBox */
            CRegion(const CBox& box);
            /* Create from a pixman_box32_t */
            CRegion(pixman_box32_t* box);

            CRegion(const CRegion&);
            CRegion(CRegion&&) noexcept;

            ~CRegion();

            CRegion& operator=(CRegion&& other) noexcept {
                if (this != &other)
                    pixman_region32_copy(&m_rRegion, other.pixman());

                return *this;
            }

            CRegion& operator=(const CRegion& other) {
                if (this != &other)
                    pixman_region32_copy(&m_rRegion, other.pixman());

                return *this;
            }

            CRegion&                    clear();
            CRegion&                    set(const CRegion& other);
            CRegion&                    add(const CRegion& other);
            CRegion&                    add(double x, double y, double w, double h);
            CRegion&                    add(const CBox& other);
            CRegion&                    subtract(const CRegion& other);
            CRegion&                    intersect(const CRegion& other);
            CRegion&                    intersect(double x, double y, double w, double h);
            CRegion&                    translate(const Vector2D& vec);
            CRegion&                    transform(const eTransform t, double w, double h);
            CRegion&                    invert(pixman_box32_t* box);
            CRegion&                    invert(const CBox& box);
            CRegion&                    scale(float scale);
            CRegion&                    scale(const Vector2D& scale);
            CRegion&                    expand(double units);
            CRegion&                    rationalize();
            CBox                        getExtents();
            bool                        containsPoint(const Vector2D& vec) const;
            bool                        empty() const;
            Vector2D                    closestPoint(const Vector2D& vec) const;
            CRegion                     copy() const;

            std::vector<pixman_box32_t> getRects() const;
            std::vector<pixman_box32_t> getRectsSimplified() const;

            template <typename T>
            void forEachRect(T&& cb) const {
                int         rectsNum = 0;
                const auto* rects    = pixman_region32_rectangles(&m_rRegion, &rectsNum);
                for (int i = 0; i < rectsNum; ++i) {
                    std::forward<T>(cb)(rects[i]);
                }
            }

            template <typename T>
            void forEachRectSimplified(T&& cb) const {
                pixman_region32_t reduced;
                pixman_region32_init(&reduced);
                pixman_region32_union(&reduced, &m_rRegion, &m_rRegion);

                int   n     = 0;
                auto* rects = pixman_region32_rectangles(&reduced, &n);
                for (int i = 0; i < n; ++i)
                    std::forward<T>(cb)(rects[i]);

                pixman_region32_fini(&reduced);
            }

            //
            pixman_region32_t* pixman() {
                return &m_rRegion;
            }

            const pixman_region32_t* pixman() const {
                return &m_rRegion;
            }

          private:
            pixman_region32_t m_rRegion;
        };
    }
}
