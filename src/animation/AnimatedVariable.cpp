#include <hyprutils/animation/AnimatedVariable.hpp>
#include <hyprutils/animation/AnimationManager.hpp>

using namespace Hyprutils::Animation;

void CBaseAnimatedVariable::create(Hyprutils::Animation::CAnimationManager* pAnimationManager, int typeInfo) {
    m_pAnimationManager = pAnimationManager;
    m_Type              = typeInfo;

    m_bDummy = false;
}

void CBaseAnimatedVariable::connectToActive() {
    if (!m_pAnimationManager || m_bDummy)
        return;

    m_pAnimationManager->scheduleTick(); // otherwise the animation manager will never pick this up
    if (!m_bIsConnectedToActive)
        m_pAnimationManager->m_vActiveAnimatedVariables.push_back(this);
    m_bIsConnectedToActive = true;
}

void CBaseAnimatedVariable::disconnectFromActive() {
    if (!m_pAnimationManager)
        return;

    std::erase_if(m_pAnimationManager->m_vActiveAnimatedVariables, [&](const auto& other) { return other == this; });
    m_bIsConnectedToActive = false;
}

bool Hyprutils::Animation::CBaseAnimatedVariable::enabled() const {
    return m_pConfig ? m_pConfig->pValues->internalEnabled : false;
}

const std::string& CBaseAnimatedVariable::getBezierName() const {
    static constexpr const std::string DEFAULTBEZIERNAME = "default";
    return m_pConfig ? m_pConfig->pValues->internalBezier : DEFAULTBEZIERNAME;
}

const std::string& CBaseAnimatedVariable::getStyle() const {
    static constexpr const std::string DEFAULTSTYLE = "";
    return m_pConfig ? m_pConfig->pValues->internalStyle : DEFAULTSTYLE;
}

float CBaseAnimatedVariable::getPercent() const {
    const auto DURATIONPASSED = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - animationBegin).count();
    return std::clamp((DURATIONPASSED / 100.f) / m_pConfig->pValues->internalSpeed, 0.f, 1.f);
}

float CBaseAnimatedVariable::getCurveValue() const {
    if (!m_bIsBeingAnimated || !m_pAnimationManager)
        return 1.f;

    const auto SPENT = getPercent();

    if (SPENT >= 1.f)
        return 1.f;

    return m_pAnimationManager->getBezier(m_pConfig->pValues->internalBezier)->getYForPoint(SPENT);
}

bool CBaseAnimatedVariable::ok() const {
    return m_pConfig && m_pAnimationManager;
}

void CBaseAnimatedVariable::onUpdate() {
    if (m_fUpdateCallback)
        m_fUpdateCallback(this);
}

void CBaseAnimatedVariable::setCallbackOnEnd(CallbackFun func, bool remove) {
    m_fEndCallback       = std::move(func);
    m_bRemoveEndAfterRan = remove;

    if (!isBeingAnimated())
        onAnimationEnd();
}

void CBaseAnimatedVariable::setCallbackOnBegin(CallbackFun func, bool remove) {
    m_fBeginCallback       = std::move(func);
    m_bRemoveBeginAfterRan = remove;
}

void CBaseAnimatedVariable::setUpdateCallback(CallbackFun func) {
    m_fUpdateCallback = std::move(func);
}

void CBaseAnimatedVariable::resetAllCallbacks() {
    m_fBeginCallback       = nullptr;
    m_fEndCallback         = nullptr;
    m_fUpdateCallback      = nullptr;
    m_bRemoveBeginAfterRan = false;
    m_bRemoveEndAfterRan   = false;
}

void CBaseAnimatedVariable::onAnimationEnd() {
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

void CBaseAnimatedVariable::onAnimationBegin() {
    m_bIsBeingAnimated = true;
    animationBegin     = std::chrono::steady_clock::now();
    connectToActive();

    if (m_fBeginCallback) {
        m_fBeginCallback(this);
        if (m_bRemoveBeginAfterRan)
            m_fBeginCallback = nullptr; // reset
    }
}
