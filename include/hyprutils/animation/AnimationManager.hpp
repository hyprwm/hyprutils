#pragma once

#include "./BezierCurve.hpp"
#include "../math/Vector2D.hpp"
#include "../memory/WeakPtr.hpp"
#include "../signal/Signal.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Hyprutils {
    namespace Animation {
        class CBaseAnimatedVariable;

        struct SSpringCurve {
            float stiffness       = 250.F;
            float damping         = 25.F;
            float mass            = 1.F;
            float valueEpsilon    = 0.001F;
            float velocityEpsilon = 0.001F;
        };

        /* A class for managing bezier curves and variables that are being animated. */
        class CAnimationManager {
          public:
            CAnimationManager();
            virtual ~CAnimationManager() = default;

            void                                                                         tickDone();
            void                                                                         rotateActive();
            bool                                                                         shouldTickForNext();

            virtual void                                                                 scheduleTick() = 0;
            virtual void                                                                 onTicked()     = 0;

            void                                                                         addBezierWithName(std::string, const Math::Vector2D&, const Math::Vector2D&);
            void                                                                         removeAllBeziers();
            void                                                                         addSpringWithName(std::string, const SSpringCurve&);
            void                                                                         removeAllSprings();

            bool                                                                         bezierExists(const std::string&);
            bool                                                                         springExists(const std::string&);
            Memory::CSharedPointer<CBezierCurve>                                         getBezier(const std::string&);
            Memory::CSharedPointer<SSpringCurve>                                         getSpring(const std::string&);

            const std::unordered_map<std::string, Memory::CSharedPointer<CBezierCurve>>& getAllBeziers();
            const std::unordered_map<std::string, Memory::CSharedPointer<SSpringCurve>>& getAllSprings();

            struct SAnimationManagerSignals {
                Signal::CSignalT<Memory::CWeakPointer<CBaseAnimatedVariable>> connect;
                Signal::CSignalT<Memory::CWeakPointer<CBaseAnimatedVariable>> disconnect;
            };

            Memory::CWeakPointer<SAnimationManagerSignals>           getSignals() const;

            std::vector<Memory::CWeakPointer<CBaseAnimatedVariable>> m_vActiveAnimatedVariables;

          private:
            std::unordered_map<std::string, Memory::CSharedPointer<CBezierCurve>> m_mBezierCurves;
            std::unordered_map<std::string, Memory::CSharedPointer<SSpringCurve>> m_mSpringCurves;

            bool                                                                  m_bTickScheduled = false;

            struct SAnimVarListeners {
                Signal::CHyprSignalListener connect;
                Signal::CHyprSignalListener disconnect;
            };

            Memory::CUniquePointer<SAnimVarListeners>        m_listeners;
            Memory::CUniquePointer<SAnimationManagerSignals> m_events;
        };
    }
}
