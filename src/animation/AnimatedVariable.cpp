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
