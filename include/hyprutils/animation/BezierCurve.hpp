#pragma once

#include <array>
#include <vector>

#include "../math/Vector2D.hpp"

constexpr int   BAKEDPOINTS    = 255;
constexpr float INVBAKEDPOINTS = 1.f / BAKEDPOINTS;

namespace Hyprutils {
    namespace Animation {

        /* An implementation of a cubic bezier curve. */
        class CBezierCurve {
          public:
            /* Calculates a cubic bezier curve based on 2 control points (EXCLUDES the 0,0 and 1,1 points). */
            void  setup(const std::array<Hyprutils::Math::Vector2D, 2>& points);

            float getYForT(float const& t) const;
            float getXForT(float const& t) const;
            float getYForPoint(float const& x) const;

          private:
            /* this INCLUDES the 0,0 and 1,1 points. */
            std::vector<Hyprutils::Math::Vector2D>             m_vPoints;

            std::array<Hyprutils::Math::Vector2D, BAKEDPOINTS> m_aPointsBaked;
        };
    }
}
