#include <hyprutils/animation/AnimationManager.hpp>
#include <hyprutils/animation/AnimatedVariable.hpp>

using namespace Hyprutils::Animation;
using namespace Hyprutils::Math;
using namespace Hyprutils::Memory;
using namespace Hyprutils::Signal;

#define SP CSharedPointer
#define WP CWeakPointer

const std::array<Vector2D, 2> DEFAULTBEZIERPOINTS = {Vector2D(0.0, 0.75), Vector2D(0.15, 1.0)};

CAnimationManager::CAnimationManager() {
    const auto BEZIER = makeShared<CBezierCurve>();
    BEZIER->setup(DEFAULTBEZIERPOINTS);
    m_mBezierCurves["default"] = BEZIER;

    m_events    = makeUnique<SAnimationManagerSignals>();
    m_listeners = makeUnique<SAnimVarListeners>();

    m_listeners->connect    = m_events->connect.registerListener([this](std::any data) { onConnect(data); });
    m_listeners->disconnect = m_events->disconnect.registerListener([this](std::any data) { onDisconnect(data); });
}

void CAnimationManager::onConnect(std::any data) {
    if (!m_bTickScheduled)
        scheduleTick();

    try {
        const auto PAV = std::any_cast<WP<CBaseAnimatedVariable>>(data);
        if (!PAV)
            return;

        m_vActiveAnimatedVariables.emplace_back(PAV);
    } catch (const std::bad_any_cast&) { return; }
}

void CAnimationManager::onDisconnect(std::any data) {
    try {
        const auto PAV = std::any_cast<WP<CBaseAnimatedVariable>>(data);
        if (!PAV)
            return;

        std::erase_if(m_vActiveAnimatedVariables, [&](const auto& other) { return !other || other == PAV; });
    } catch (const std::bad_any_cast&) { return; }
}

void CAnimationManager::removeAllBeziers() {
    m_mBezierCurves.clear();

    // add the default one
    const auto BEZIER = makeShared<CBezierCurve>();
    BEZIER->setup(DEFAULTBEZIERPOINTS);
    m_mBezierCurves["default"] = BEZIER;
}

void CAnimationManager::addBezierWithName(std::string name, const Vector2D& p1, const Vector2D& p2) {
    const auto BEZIER = makeShared<CBezierCurve>();
    BEZIER->setup({
        p1,
        p2,
    });
    m_mBezierCurves[name] = BEZIER;
}

bool CAnimationManager::shouldTickForNext() {
    return !m_vActiveAnimatedVariables.empty();
}

void CAnimationManager::tickDone() {
    rotateActive();
}

void CAnimationManager::rotateActive() {
    std::vector<CWeakPointer<CBaseAnimatedVariable>> active;
    active.reserve(m_vActiveAnimatedVariables.size()); // avoid reallocations
    for (auto const& av : m_vActiveAnimatedVariables) {
        const auto PAV = av.lock();
        if (!PAV)
            continue;

        if (PAV->ok() && PAV->isBeingAnimated())
            active.emplace_back(av);
        else
            PAV->m_bIsConnectedToActive = false;
    }

    m_vActiveAnimatedVariables = std::move(active);
}

bool CAnimationManager::bezierExists(const std::string& bezier) {
    for (auto const& [bc, bz] : m_mBezierCurves) {
        if (bc == bezier)
            return true;
    }

    return false;
}

SP<CBezierCurve> CAnimationManager::getBezier(const std::string& name) {
    const auto BEZIER = std::ranges::find_if(m_mBezierCurves, [&](const auto& other) { return other.first == name; });

    return BEZIER == m_mBezierCurves.end() ? m_mBezierCurves["default"] : BEZIER->second;
}

const std::unordered_map<std::string, SP<CBezierCurve>>& CAnimationManager::getAllBeziers() {
    return m_mBezierCurves;
}

CWeakPointer<CAnimationManager::SAnimationManagerSignals> CAnimationManager::getSignals() const {
    return m_events;
}
