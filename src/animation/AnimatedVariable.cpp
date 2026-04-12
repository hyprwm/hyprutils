#include <hyprutils/animation/AnimatedVariable.hpp>
#include <hyprutils/animation/AnimationManager.hpp>
#include <hyprutils/memory/WeakPtr.hpp>

#include <algorithm>
#include <cmath>

using namespace Hyprutils::Animation;
using namespace Hyprutils::Memory;

static const std::string          DEFAULTBEZIERNAME = "default";
static const std::string          DEFAULTSTYLE      = "";
static constexpr std::string_view SPRINGPREFIX      = "spring:";

#define SP CSharedPointer
#define WP CWeakPointer

void CBaseAnimatedVariable::create(CAnimationManager* pManager, int typeInfo, SP<CBaseAnimatedVariable> pSelf) {
    m_Type  = typeInfo;
    m_pSelf = std::move(pSelf);

    m_pAnimationManager = pManager;
    m_pSignals          = pManager->getSignals();
    m_bDummy            = false;
}

void CBaseAnimatedVariable::create2(CAnimationManager* pManager, int typeInfo, WP<CBaseAnimatedVariable> pSelf) {
    m_Type  = typeInfo;
    m_pSelf = std::move(pSelf);

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
    if (m_pConfig && m_pConfig->pValues)
        return m_pConfig->pValues->internalEnabled;

    return false;
}

const std::string& CBaseAnimatedVariable::getBezierName() const {
    if (m_pConfig && m_pConfig->pValues)
        return m_pConfig->pValues->internalBezier;

    return DEFAULTBEZIERNAME;
}

const std::string& CBaseAnimatedVariable::getStyle() const {
    if (m_pConfig && m_pConfig->pValues)
        return m_pConfig->pValues->internalStyle;

    return DEFAULTSTYLE;
}

float CBaseAnimatedVariable::getPercent() const {
    const auto DURATIONPASSED = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - animationBegin).count();

    if (m_pConfig && m_pConfig->pValues)
        return std::clamp((DURATIONPASSED / 100.F) / m_pConfig->pValues->internalSpeed, 0.f, 1.f);

    return 1.F;
}

float CBaseAnimatedVariable::getCurveValue() const {
    if (!m_bIsBeingAnimated || isAnimationManagerDead())
        return 1.F;

    if (isSpringCurve())
        return m_fSpringValue;

    const auto BEZIER = m_pAnimationManager->getBezier(getBezierName());
    if (!BEZIER)
        return 1.F;

    const auto SPENT = getPercent();
    if (SPENT >= 1.F)
        return 1.F;

    return BEZIER->getYForPoint(SPENT);
}

CBaseAnimatedVariable::SCurveStepResult CBaseAnimatedVariable::getCurveStep() {
    if (!m_bIsBeingAnimated || isAnimationManagerDead())
        return {};

    if (!isSpringCurve()) {
        const auto SPENT = getPercent();
        if (SPENT >= 1.f)
            return {.value = 1.F, .finished = true};

        const auto BEZIER = m_pAnimationManager->getBezier(getBezierName());
        if (!BEZIER)
            return {.value = 1.F, .finished = true};

        return {
            .value    = BEZIER->getYForPoint(SPENT),
            .finished = false,
        };
    }

    const auto SPRINGNAME = springNameFromSpec(getBezierName());
    const auto SPRING     = m_pAnimationManager->getSpring(std::string{SPRINGNAME});
    if (!SPRING)
        return {.value = 1.F, .finished = true};

    const auto NOW = std::chrono::steady_clock::now();
    float      dt  = std::chrono::duration<float>(NOW - springLastStep).count();
    springLastStep = NOW;

    constexpr float MINDELTA = 1.F / 240.F;
    if (dt <= 0.F)
        dt = MINDELTA;
    else
        dt = std::clamp(dt, MINDELTA, 0.05F);

    if (dt > 0.F) {
        constexpr const float FIXEDSTEP = 1.F / 240.F;
        const int             SUBSTEPS  = std::clamp(static_cast<int>(std::ceil(dt / FIXEDSTEP)), 1, 16);
        const float           STEPTIME  = dt / SUBSTEPS;
        const float           MASS      = std::max(SPRING->mass, 0.0001f);

        for (int i = 0; i < SUBSTEPS; ++i) {
            const float displacement = m_fSpringValue - 1.f;
            const float acceleration = ((-SPRING->stiffness * displacement) - (SPRING->damping * m_fSpringVelocity)) / MASS;

            m_fSpringVelocity += acceleration * STEPTIME;
            m_fSpringValue += m_fSpringVelocity * STEPTIME;
        }
    }

    const bool FINISHED = std::abs(1.F - m_fSpringValue) <= SPRING->valueEpsilon && std::abs(m_fSpringVelocity) <= SPRING->velocityEpsilon;
    if (FINISHED) {
        m_fSpringValue    = 1.F;
        m_fSpringVelocity = 0.F;
    }

    return {.value = m_fSpringValue, .finished = FINISHED};
}

bool CBaseAnimatedVariable::isSpringCurve() const {
    return !springNameFromSpec(getBezierName()).empty();
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

void CBaseAnimatedVariable::onAnimationBegin(bool preserveCurveState, float springVelocityScale) {
    if (isSpringCurve())
        resetSpringState(preserveCurveState, springVelocityScale);

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

void CBaseAnimatedVariable::resetSpringState(bool preserveVelocity, float velocityScale) {
    m_fSpringValue = 0.f;
    if (!preserveVelocity)
        m_fSpringVelocity = 0.f;
    else
        m_fSpringVelocity *= velocityScale;

    springLastStep = std::chrono::steady_clock::now();
}

std::string_view CBaseAnimatedVariable::springNameFromSpec(const std::string& spec) const {
    if (!spec.starts_with(SPRINGPREFIX) || spec.size() <= SPRINGPREFIX.size())
        return {};

    return std::string_view(spec).substr(SPRINGPREFIX.size());
}
