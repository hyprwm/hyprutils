#include <hyprutils/animation/AnimationManager.hpp>

using namespace Hyprutils::Animation;
using namespace Hyprutils::Math;
using namespace Hyprutils::Memory;
using namespace Hyprutils::Signal;

#define SP CSharedPointer

const std::array<Vector2D, 2> DEFAULTBEZIERPOINTS = {Vector2D(0.0, 0.75), Vector2D(0.15, 1.0)};

CAnimationManager::CAnimationManager() {
    const auto BEZIER = makeShared<CBezierCurve>();
    BEZIER->setup(DEFAULTBEZIERPOINTS);
    m_mBezierCurves["default"] = BEZIER;

    m_sEvents.connect         = makeShared<CSignal>();
    m_sEvents.forceDisconnect = makeShared<CSignal>();
    m_sEvents.lazyDisconnect  = makeShared<CSignal>();

    m_sListeners.connect         = m_sEvents.connect->registerListener([this](std::any data) { connectListener(data); });
    m_sListeners.forceDisconnect = m_sEvents.forceDisconnect->registerListener([this](std::any data) { forceDisconnectListener(data); });
    m_sListeners.lazyDisconnect  = m_sEvents.lazyDisconnect->registerListener([this](std::any data) { lazyDisconnectListener(data); });
}

void CAnimationManager::connectListener(std::any data) {
    if (!m_bTickScheduled)
        scheduleTick();

    try {
        const auto PAV = std::any_cast<SP<CBaseAnimatedVariable>>(data);
        if (!PAV)
            return;

        m_vActiveAnimatedVariables.emplace_back(PAV);
    } catch (const std::bad_any_cast&) { return; }

    // When the animation manager ticks, it will cleanup the active list.
    // If for some reason we don't tick for a while, but vars get warped a lot, we could end up with a lot of pending disconnects.
    // So we rorate here, since we don't want the vector to grow too big for no reason.
    if (m_pendingDisconnects > 100)
        rotateActive();
}

void CAnimationManager::forceDisconnectListener(std::any data) {
    try {
        const auto PAV = std::any_cast<void*>(data);
        if (!PAV)
            return;

        std::erase_if(m_vActiveAnimatedVariables, [&](const auto& other) { return other.get() == PAV; });
    } catch (const std::bad_any_cast&) { return; }
}

void CAnimationManager::lazyDisconnectListener(std::any data) {
    m_pendingDisconnects++;
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
    m_pendingDisconnects       = 0;
}

bool CAnimationManager::bezierExists(const std::string& bezier) {
    for (auto const& [bc, bz] : m_mBezierCurves) {
        if (bc == bezier)
            return true;
    }

    return false;
}

SP<CBezierCurve> CAnimationManager::getBezier(const std::string& name) {
    const auto BEZIER = std::find_if(m_mBezierCurves.begin(), m_mBezierCurves.end(), [&](const auto& other) { return other.first == name; });

    return BEZIER == m_mBezierCurves.end() ? m_mBezierCurves["default"] : BEZIER->second;
}

const std::unordered_map<std::string, SP<CBezierCurve>>& CAnimationManager::getAllBeziers() {
    return m_mBezierCurves;
}
