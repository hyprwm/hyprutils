#pragma once

#include "./BezierCurve.hpp"
#include "./AnimatedVariable.hpp"
#include "../math/Vector2D.hpp"
#include "../memory/SharedPtr.hpp"

#include <unordered_map>
#include <vector>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;

namespace Hyprutils {
    namespace Animation {

        /* A class for managing bezier curves and variables that are being animated. */
        class CAnimationManager {
          public:
            CAnimationManager();

            void                                                          tickDone();
            bool                                                          shouldTickForNext();

            virtual void                                                  scheduleTick() = 0;
            virtual void                                                  onTicked()     = 0;

            void                                                          addBezierWithName(std::string, const Vector2D&, const Vector2D&);
            void                                                          removeAllBeziers();

            bool                                                          bezierExists(const std::string&);
            CSharedPointer<CBezierCurve>                                  getBezier(const std::string&);

            std::unordered_map<std::string, CSharedPointer<CBezierCurve>> getAllBeziers();

            std::vector<CBaseAnimatedVariable*>                           m_vActiveAnimatedVariables;

          private:
            std::unordered_map<std::string, CSharedPointer<CBezierCurve>> m_mBezierCurves;

            bool                                                          m_bTickScheduled = false;
        };
    }
}
