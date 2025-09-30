#pragma once

#include <array>
#include <vector>

#include "../math/Vector2D.hpp"

namespace Hyprutils {
    namespace Animation {
        constexpr int   BAKEDPOINTS    = 255;
        constexpr float INVBAKEDPOINTS = 1.f / BAKEDPOINTS;

        /* An implementation of a cubic bezier curve. */
        class CBezierCurve {
          public:
            /* Calculates a cubic bezier curve based on 2 control points (EXCLUDES the 0,0 and 1,1 points). */
            void setup(const std::array<Hyprutils::Math::Vector2D, 2>& points);
            /* Calculates a cubic bezier curve based on 4 control points. */
            void  setup4(const std::array<Hyprutils::Math::Vector2D, 4>& points);

            float getYForT(float const& t) const;
            float getXForT(float const& t) const;
            float getYForPoint(float const& x) const;

            /* this INCLUDES the 0,0 and 1,1 points. */
            const std::vector<Hyprutils::Math::Vector2D>& getControlPoints() const;

          private:
            /* this INCLUDES the 0,0 and 1,1 points. */
            std::vector<Hyprutils::Math::Vector2D>             m_vPoints;

            std::array<Hyprutils::Math::Vector2D, BAKEDPOINTS> m_aPointsBaked;
        };
    }
}
