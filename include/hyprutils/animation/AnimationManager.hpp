#pragma once

#include "./BezierCurve.hpp"
#include "./AnimatedVariable.hpp"
#include "../math/Vector2D.hpp"
#include "../memory/WeakPtr.hpp"
#include "../signal/Signal.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Hyprutils {
    namespace Animation {
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

            bool                                                                         bezierExists(const std::string&);
            Memory::CSharedPointer<CBezierCurve>                                         getBezier(const std::string&);

            const std::unordered_map<std::string, Memory::CSharedPointer<CBezierCurve>>& getAllBeziers();

            std::vector<Memory::CWeakPointer<CBaseAnimatedVariable>>                     m_vActiveAnimatedVariables;

          private:
            std::unordered_map<std::string, Memory::CSharedPointer<CBezierCurve>> m_mBezierCurves;

            bool                                                                  m_bTickScheduled     = false;
            uint32_t                                                              m_pendingDisconnects = 0;

            void                                                                  connectListener(std::any data);
            void                                                                  lazyDisconnectListener(std::any data);
            void                                                                  forceDisconnectListener(std::any data);

            struct {
                Signal::CHyprSignalListener connect;
                Signal::CHyprSignalListener forceDisconnect;
                Signal::CHyprSignalListener lazyDisconnect;
            } m_sListeners;

            Memory::CSharedPointer<SAnimVarEvents> m_events;

            friend class CBaseAnimatedVariable;
        };
    }
}
