#pragma once

#include "AnimationConfig.hpp"
#include "../memory/WeakPtr.hpp"
#include "../memory/SharedPtr.hpp"
#include "../signal/Signal.hpp"
#include "AnimationManager.hpp"

#include <functional>
#include <chrono>
#include <cmath>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace Hyprutils {
    namespace Animation {

        /* A base class for animated variables. */
        class CBaseAnimatedVariable {
          public:
            using CallbackFun = std::function<void(Memory::CWeakPointer<CBaseAnimatedVariable> thisptr)>;

            struct SCurveStepResult {
                float value    = 1.f;
                bool  finished = true;
            };

            CBaseAnimatedVariable() {
                ; // m_bDummy = true;
            };

            void create(CAnimationManager*, int, Memory::CSharedPointer<CBaseAnimatedVariable>);
            void create2(CAnimationManager*, int, Memory::CWeakPointer<CBaseAnimatedVariable>);
            void connectToActive();
            void disconnectFromActive();

            /* Needs to call disconnectFromActive to remove `m_pSelf` from the active animation list */
            virtual ~CBaseAnimatedVariable() {
                disconnectFromActive();
            };

            virtual void warp(bool endCallback = true, bool forceDisconnect = true) = 0;

            CBaseAnimatedVariable(const CBaseAnimatedVariable&)            = delete;
            CBaseAnimatedVariable(CBaseAnimatedVariable&&)                 = delete;
            CBaseAnimatedVariable& operator=(const CBaseAnimatedVariable&) = delete;
            CBaseAnimatedVariable& operator=(CBaseAnimatedVariable&&)      = delete;

            //
            void setConfig(Memory::CSharedPointer<SAnimationPropertyConfig> pConfig) {
                m_pConfig = pConfig;
            }

            Memory::CWeakPointer<SAnimationPropertyConfig> getConfig() const {
                return m_pConfig;
            }

            bool               enabled() const;
            const std::string& getBezierName() const;
            const std::string& getStyle() const;

            /* returns the spent (completion) % */
            float getPercent() const;

            /* returns the current curve value. */
            float getCurveValue() const;

            /* steps the current curve by one frame. */
            SCurveStepResult getCurveStep();

            bool             isSpringCurve() const;

            /* checks if an animation is in progress */
            bool isBeingAnimated() const {
                return m_bIsBeingAnimated;
            }

            /* checks m_bDummy and m_pAnimationManager */
            bool ok() const;

            /* calls the update callback */
            void onUpdate();

            /* sets a function to be ran when an animation ended.
               if "remove" is set to true, it will remove the callback when ran. */
            void setCallbackOnEnd(CallbackFun func, bool remove = true);

            /* sets a function to be ran when an animation is started.
               if "remove" is set to true, it will remove the callback when ran. */
            void setCallbackOnBegin(CallbackFun func, bool remove = true);

            /* sets the update callback, called every time the value is animated and a step is done
               Warning: calling unregisterVar/registerVar in this handler will cause UB */
            void setUpdateCallback(CallbackFun func);

            /* resets all callbacks. Does not call any. */
            void resetAllCallbacks();

            void onAnimationEnd();
            void onAnimationBegin(bool preserveCurveState = false, float springVelocityScale = 1.F);

            /* returns whether the parent CAnimationManager is dead */
            bool isAnimationManagerDead() const;

            int  m_Type = -1;

          protected:
            friend class CAnimationManager;

            CAnimationManager*                                                m_pAnimationManager = nullptr;

            bool                                                              m_bIsConnectedToActive = false;
            bool                                                              m_bIsBeingAnimated     = false;

            Memory::CWeakPointer<CBaseAnimatedVariable>                       m_pSelf;

            Memory::CWeakPointer<CAnimationManager::SAnimationManagerSignals> m_pSignals;

          private:
            void                                           resetSpringState(bool preserveVelocity, float velocityScale);
            std::string_view                               springNameFromSpec(const std::string& spec) const;

            Memory::CWeakPointer<SAnimationPropertyConfig> m_pConfig;

            std::chrono::steady_clock::time_point          animationBegin;
            std::chrono::steady_clock::time_point          springLastStep;

            bool                                           m_bDummy = true;

            float                                          m_fSpringValue    = 1.F;
            float                                          m_fSpringVelocity = 0.F;

            bool                                           m_bRemoveEndAfterRan   = true;
            bool                                           m_bRemoveBeginAfterRan = true;

            CallbackFun                                    m_fEndCallback;
            CallbackFun                                    m_fBeginCallback;
            CallbackFun                                    m_fUpdateCallback;
        };

        /* This concept represents the minimum requirement for a type to be used with CGenericAnimatedVariable */
        template <class ValueImpl>
        concept AnimatedType = requires(ValueImpl val) {
            requires std::is_copy_constructible_v<ValueImpl>;
            { val == val } -> std::same_as<bool>; // requires operator==
            { val = val };                        // requires operator=
        };

        template <class ValueImpl>
        concept AnimableType = AnimatedType<ValueImpl> && requires(ValueImpl val, float pointy) {
            { val - val };
            { val + ((val - val) * pointy) } -> std::convertible_to<ValueImpl>;
        };

        /*
            A generic class for variables.
            VarType is the type of the variable to be animated.
            AnimationContext is there to attach additional data to the animation.
            In Hyprland that struct would contain a reference to window, workspace or layer for example.
        */
        template <AnimatedType VarType, class AnimationContext>
        class CGenericAnimatedVariable : public CBaseAnimatedVariable {
          public:
            CGenericAnimatedVariable() = default;

            /* Deprecated: use create2 */
            void create(const int typeInfo, CAnimationManager* pAnimationManager, Memory::CSharedPointer<CGenericAnimatedVariable<VarType, AnimationContext>> pSelf,
                        const VarType& initialValue) {
                m_Begun = initialValue;
                m_Value = initialValue;
                m_Goal  = initialValue;

                CBaseAnimatedVariable::create(pAnimationManager, typeInfo, pSelf);
            }

            /* Equivalent to create, except that it allows animated variables to be UP's */
            void create2(const int typeInfo, CAnimationManager* pAnimationManager, Memory::CWeakPointer<CGenericAnimatedVariable<VarType, AnimationContext>> pSelf,
                         const VarType& initialValue) {
                m_Begun = initialValue;
                m_Value = initialValue;
                m_Goal  = initialValue;

                CBaseAnimatedVariable::create2(pAnimationManager, typeInfo, pSelf);
            }

            CGenericAnimatedVariable(const CGenericAnimatedVariable&)            = delete;
            CGenericAnimatedVariable(CGenericAnimatedVariable&&)                 = delete;
            CGenericAnimatedVariable& operator=(const CGenericAnimatedVariable&) = delete;
            CGenericAnimatedVariable& operator=(CGenericAnimatedVariable&&)      = delete;

            virtual void              warp(bool endCallback = true, bool forceDisconnect = true) {
                if (!m_bIsBeingAnimated)
                    return;

                m_Value = m_Goal;

                onUpdate();

                m_bIsBeingAnimated = false;

                if (forceDisconnect)
                    disconnectFromActive();

                if (endCallback)
                    onAnimationEnd();
            }

            const VarType& value() const {
                return m_Value;
            }

            /* used to update the value each tick via the AnimationManager */
            VarType& value() {
                return m_Value;
            }

            const VarType& goal() const {
                return m_Goal;
            }

            const VarType& begun() const {
                return m_Begun;
            }

            CGenericAnimatedVariable& operator=(const VarType& v) {
                if (v == m_Goal)
                    return *this;

                const bool WASANIMATING        = m_bIsBeingAnimated;
                float      SPRINGVELOCITYSCALE = 1.f;

                if (WASANIMATING && isSpringCurve()) {
                    if constexpr (std::is_arithmetic_v<VarType>) {
                        const float OLDDELTA = static_cast<float>(m_Goal - m_Begun);
                        const float NEWDELTA = static_cast<float>(v - m_Value);

                        if (std::abs(NEWDELTA) > 1e-6f)
                            SPRINGVELOCITYSCALE = OLDDELTA / NEWDELTA;
                        else
                            SPRINGVELOCITYSCALE = 0.f;
                    }
                }

                m_Goal  = v;
                m_Begun = m_Value;

                onAnimationBegin(WASANIMATING && isSpringCurve(), SPRINGVELOCITYSCALE);

                return *this;
            }

            /* Sets the actual stored value, without affecting the goal, but resets the timer*/
            void setValue(const VarType& v) {
                if (v == m_Value)
                    return;

                m_Value = v;
                m_Begun = m_Value;

                onAnimationBegin();
            }

            /* Sets the actual value and goal*/
            void setValueAndWarp(const VarType& v) {
                m_Goal             = v;
                m_bIsBeingAnimated = true;

                warp();
            }

            template <class T = VarType>
                requires AnimableType<T>
            SCurveStepResult update(bool warpNow = false) {
                if (warpNow || m_Value == m_Goal || !enabled()) {
                    warp(true, false);
                    return SCurveStepResult{.value = 1.F, .finished = true};
                }

                const auto STEP = getCurveStep();
                if (STEP.finished) {
                    warp(true, false);
                    return STEP;
                }

                const auto DELTA = m_Goal - m_Begun;
                m_Value          = m_Begun + (DELTA * STEP.value);

                onUpdate();

                return STEP;
            }

            AnimationContext m_Context;

          private:
            VarType m_Value{};
            VarType m_Goal{};
            VarType m_Begun{};
        };
    }
}
