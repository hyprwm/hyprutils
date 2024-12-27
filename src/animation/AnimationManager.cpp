#include <hyprutils/animation/AnimationManager.hpp>

using namespace Hyprutils::Animation;
using namespace Hyprutils::Math;
using namespace Hyprutils::Memory;

#define SP CSharedPointer

const std::array<Vector2D, 2> DEFAULTBEZIERPOINTS = {Vector2D(0.0, 0.75), Vector2D(0.15, 1.0)};

CAnimationManager::CAnimationManager() {
    const auto BEZIER = makeShared<CBezierCurve>();
    BEZIER->setup(DEFAULTBEZIERPOINTS);
    m_mBezierCurves["default"] = BEZIER;
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
    std::vector<CBaseAnimatedVariable*> active;
    // avoid reallocations
    active.reserve(m_vActiveAnimatedVariables.size());
    for (auto const& av : m_vActiveAnimatedVariables) {
        if (av->ok() && av->isBeingAnimated())
            active.push_back(av);
        else
            av->m_bIsConnectedToActive = false;
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
    const auto BEZIER = std::find_if(m_mBezierCurves.begin(), m_mBezierCurves.end(), [&](const auto& other) { return other.first == name; });

    return BEZIER == m_mBezierCurves.end() ? m_mBezierCurves["default"] : BEZIER->second;
}

std::unordered_map<std::string, SP<CBezierCurve>> CAnimationManager::getAllBeziers() {
    return m_mBezierCurves;
}
