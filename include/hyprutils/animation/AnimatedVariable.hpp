#pragma once

#include <functional>
#include <chrono>

namespace Hyprutils {
    namespace Animation {
        class CAnimationManager;

        /*
            Structure for animation properties.
            Config properties need to have a static lifetime to allow for config reload.
        */
        struct SAnimationPropertyConfig {
            bool                      overridden = true;

            std::string               internalBezier  = "";
            std::string               internalStyle   = "";
            float                     internalSpeed   = 0.f;
            int                       internalEnabled = -1;

            SAnimationPropertyConfig* pValues          = nullptr;
            SAnimationPropertyConfig* pParentAnimation = nullptr;
        };

        static const std::string DEFAULTSTYLE      = "";
        static const std::string DEFAULTBEZIERNAME = "default";

        /* A base class for animated variables. */
        class CBaseAnimatedVariable {
          public:
            using CallbackFun = std::function<void(CBaseAnimatedVariable* thisptr)>;

            CBaseAnimatedVariable() {
                ; // m_bDummy = true;
            };

            void create(CAnimationManager* p, int typeInfo);
            void connectToActive();
            void disconnectFromActive();

            /* Needs to call disconnectFromActive to remove `this` from the active animations */
            virtual ~CBaseAnimatedVariable() {
                disconnectFromActive();
            };

            virtual void warp(bool endCallback = true) = 0;

            CBaseAnimatedVariable(const CBaseAnimatedVariable&)            = delete;
            CBaseAnimatedVariable(CBaseAnimatedVariable&&)                 = delete;
            CBaseAnimatedVariable& operator=(const CBaseAnimatedVariable&) = delete;
            CBaseAnimatedVariable& operator=(CBaseAnimatedVariable&&)      = delete;

            void                   setConfig(SAnimationPropertyConfig* pConfig) {
                m_pConfig = pConfig;
            }

            SAnimationPropertyConfig* getConfig() const {
                return m_pConfig;
            }

            bool enabled() const {
                return m_pConfig ? m_pConfig->pValues->internalEnabled : false;
            }

            const std::string& getBezierName() const {
                return m_pConfig ? m_pConfig->pValues->internalBezier : DEFAULTBEZIERNAME;
            }

            const std::string& getStyle() const {
                return m_pConfig ? m_pConfig->pValues->internalStyle : DEFAULTSTYLE;
            }

            /* returns the spent (completion) % */
            float getPercent() const;

            /* returns the current curve value */
            float getCurveValue() const;

            /* checks if an animation is in progress */
            bool isBeingAnimated() const {
                return m_bIsBeingAnimated;
            }

            /* calls the update callback */
            void onUpdate() {
                if (m_fUpdateCallback)
                    m_fUpdateCallback(this);
            }

            bool ok() const {
                return !m_bDummy && m_pAnimationManager;
            }

            /* sets a function to be ran when the animation finishes.
               if an animation is not running, runs instantly.
               if "remove" is set to true, will remove the callback when ran. */
            void setCallbackOnEnd(CallbackFun func, bool remove = true) {
                m_fEndCallback       = std::move(func);
                m_bRemoveEndAfterRan = remove;

                if (!isBeingAnimated())
                    onAnimationEnd();
            }

            /* sets a function to be ran when an animation is started.
               if "remove" is set to true, will remove the callback when ran. */
            void setCallbackOnBegin(CallbackFun func, bool remove = true) {
                m_fBeginCallback       = std::move(func);
                m_bRemoveBeginAfterRan = remove;
            }

            /* sets the update callback, called every time the value is animated and a step is done
               Warning: calling unregisterVar/registerVar in this handler will cause UB */
            void setUpdateCallback(CallbackFun func) {
                m_fUpdateCallback = std::move(func);
            }

            /* resets all callbacks. Does not call any. */
            void resetAllCallbacks() {
                m_fBeginCallback       = nullptr;
                m_fEndCallback         = nullptr;
                m_fUpdateCallback      = nullptr;
                m_bRemoveBeginAfterRan = false;
                m_bRemoveEndAfterRan   = false;
            }

            void onAnimationEnd() {
                m_bIsBeingAnimated = false;
                /* We do not call disconnectFromActive here. The animation manager will remove it on a call to tickDone. */

                if (m_fEndCallback) {
                    /* loading m_bRemoveEndAfterRan before calling the callback allows the callback to delete this animation safely if it is false. */
                    auto removeEndCallback = m_bRemoveEndAfterRan;
                    m_fEndCallback(this);
                    if (removeEndCallback)
                        m_fEndCallback = nullptr; // reset
                }
            }

            void onAnimationBegin() {
                m_bIsBeingAnimated = true;
                animationBegin     = std::chrono::steady_clock::now();
                connectToActive();

                if (m_fBeginCallback) {
                    m_fBeginCallback(this);
                    if (m_bRemoveBeginAfterRan)
                        m_fBeginCallback = nullptr; // reset
                }
            }

            int m_Type = -1;

          protected:
            friend class CAnimationManager;

            bool m_bIsConnectedToActive = false;
            bool m_bIsBeingAnimated     = false;

          private:
            SAnimationPropertyConfig*             m_pConfig;

            std::chrono::steady_clock::time_point animationBegin;

            bool                                  m_bDummy = true;

            CAnimationManager*                    m_pAnimationManager;
            bool                                  m_bRemoveEndAfterRan   = true;
            bool                                  m_bRemoveBeginAfterRan = true;

            CallbackFun                           m_fEndCallback;
            CallbackFun                           m_fBeginCallback;
            CallbackFun                           m_fUpdateCallback;
        };

        /* This concept represents the minimum requirement for a type to be used with CGenericAnimatedVariable */
        template <class ValueImpl>
        concept AnimatedType = requires(ValueImpl val) {
            requires std::is_copy_constructible_v<ValueImpl>;
            // requires operator==
            { val == val } -> std::same_as<bool>;
            // requires operator=
            { val = val };
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

            void create(const int typeInfo, const VarType& initialValue, CAnimationManager* pAnimationManager) {
                m_Begun = initialValue;
                m_Value = initialValue;
                m_Goal  = initialValue;

                CBaseAnimatedVariable::create(pAnimationManager, typeInfo);
            }

            CGenericAnimatedVariable(const CGenericAnimatedVariable&)            = delete;
            CGenericAnimatedVariable(CGenericAnimatedVariable&&)                 = delete;
            CGenericAnimatedVariable& operator=(const CGenericAnimatedVariable&) = delete;
            CGenericAnimatedVariable& operator=(CGenericAnimatedVariable&&)      = delete;

            virtual void              warp(bool endCallback = true) {
                if (!m_bIsBeingAnimated)
                    return;

                m_Value = m_Goal;

                m_bIsBeingAnimated = false;

                onUpdate();

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

                m_Goal  = v;
                m_Begun = m_Value;

                onAnimationBegin();

                return *this;
            }

            // Sets the actual stored value, without affecting the goal, but resets the timer
            void setValue(const VarType& v) {
                if (v == m_Value)
                    return;

                m_Value = v;
                m_Begun = m_Value;

                onAnimationBegin();
            }

            // Sets the actual value and goal
            void setValueAndWarp(const VarType& v) {
                m_Goal             = v;
                m_bIsBeingAnimated = true;

                warp();
            }

            AnimationContext m_Context;

          private:
            VarType m_Value{};
            VarType m_Goal{};
            VarType m_Begun{};
        };
    }
}
