#include <hyprutils/animation/AnimatedVariable.hpp>
#include <hyprutils/animation/AnimationManager.hpp>
#include <hyprutils/memory/WeakPtr.hpp>

using namespace Hyprutils::Animation;
using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer

void CBaseAnimatedVariable::create(CAnimationManager* pManager, int typeInfo, SP<CBaseAnimatedVariable> pSelf) {
    m_Type  = typeInfo;
    m_pSelf = pSelf;

    m_pAnimationManager = pManager;
    m_pSignals          = pManager->getSignals();
    m_bDummy            = false;
}

void CBaseAnimatedVariable::connectToActive() {
    if (m_bDummy || m_bIsConnectedToActive || isAnimationManagerDead())
        return;

    m_pSignals->connect.emit(m_pSelf);
    m_bIsConnectedToActive = true;
}

void CBaseAnimatedVariable::disconnectFromActive() {
    if (isAnimationManagerDead())
        return;

    m_pSignals->disconnect.emit(m_pSelf);
    m_bIsConnectedToActive = false;
}

bool Hyprutils::Animation::CBaseAnimatedVariable::enabled() const {
    if (const auto PCONFIG = m_pConfig.lock()) {
        const auto PVALUES = PCONFIG->pValues.lock();
        return PVALUES ? PVALUES->internalEnabled : false;
    }

    return false;
}

const std::string& CBaseAnimatedVariable::getBezierName() const {
    static constexpr const std::string DEFAULTBEZIERNAME = "default";

    if (const auto PCONFIG = m_pConfig.lock()) {
        const auto PVALUES = PCONFIG->pValues.lock();
        return PVALUES ? PVALUES->internalBezier : DEFAULTBEZIERNAME;
    }

    return DEFAULTBEZIERNAME;
}

const std::string& CBaseAnimatedVariable::getStyle() const {
    static constexpr const std::string DEFAULTSTYLE = "";

    if (const auto PCONFIG = m_pConfig.lock()) {
        const auto PVALUES = PCONFIG->pValues.lock();
        return PVALUES ? PVALUES->internalStyle : DEFAULTSTYLE;
    }

    return DEFAULTSTYLE;
}

float CBaseAnimatedVariable::getPercent() const {
    const auto DURATIONPASSED = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - animationBegin).count();

    if (const auto PCONFIG = m_pConfig.lock()) {
        const auto PVALUES = PCONFIG->pValues.lock();
        return PVALUES ? std::clamp((DURATIONPASSED / 100.f) / PVALUES->internalSpeed, 0.f, 1.f) : 1.f;
    }

    return 1.f;
}

float CBaseAnimatedVariable::getCurveValue() const {
    if (!m_bIsBeingAnimated || isAnimationManagerDead())
        return 1.f;

    std::string bezierName = "";
    if (const auto PCONFIG = m_pConfig.lock()) {
        const auto PVALUES = PCONFIG->pValues.lock();
        if (PVALUES)
            bezierName = PVALUES->internalBezier;
    }

    const auto BEZIER = m_pAnimationManager->getBezier(bezierName);
    if (!BEZIER)
        return 1.f;

    const auto SPENT = getPercent();
    if (SPENT >= 1.f)
        return 1.f;

    return BEZIER->getYForPoint(SPENT);
}

bool CBaseAnimatedVariable::ok() const {
    return m_pConfig && !m_bDummy && !isAnimationManagerDead();
}

void CBaseAnimatedVariable::onUpdate() {
    if (m_bIsBeingAnimated && m_fUpdateCallback)
        m_fUpdateCallback(m_pSelf);
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
        CallbackFun cb = nullptr;
        m_fEndCallback.swap(cb);

        cb(m_pSelf);
        if (!m_bRemoveEndAfterRan && /* callback did not set a new one by itself */ !m_fEndCallback)
            m_fEndCallback = cb; // restore
    }
}

void CBaseAnimatedVariable::onAnimationBegin() {
    m_bIsBeingAnimated = true;
    animationBegin     = std::chrono::steady_clock::now();
    connectToActive();

    if (m_fBeginCallback) {
        m_fBeginCallback(m_pSelf);
        if (m_bRemoveBeginAfterRan)
            m_fBeginCallback = nullptr; // reset
    }
}

bool CBaseAnimatedVariable::isAnimationManagerDead() const {
    return m_pSignals.expired();
}
