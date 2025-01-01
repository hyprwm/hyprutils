#include <hyprutils/animation/AnimationConfig.hpp>

using namespace Hyprutils::Animation;
using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer

void CAnimationConfigTree::createNode(const std::string& nodeName, const std::string& parent) {
    auto pConfig = m_mAnimationConfig[nodeName];
    if (!pConfig)
        pConfig = makeShared<SAnimationPropertyConfig>();

    WP<SAnimationPropertyConfig> parentRef;
    if (!parent.empty() && m_mAnimationConfig.find(parent) != m_mAnimationConfig.end())
        parentRef = m_mAnimationConfig[parent];

    *pConfig = {
        .overridden       = false,
        .internalBezier   = "",
        .internalStyle    = "",
        .internalSpeed    = 0.f,
        .internalEnabled  = -1,
        .pValues          = (parentRef) ? parentRef->pValues : pConfig,
        .pParentAnimation = (parentRef) ? parentRef : pConfig,
    };

    m_mAnimationConfig[nodeName] = pConfig;
}

bool CAnimationConfigTree::nodeExists(const std::string& nodeName) const {
    return m_mAnimationConfig.find(nodeName) != m_mAnimationConfig.end();
}

void CAnimationConfigTree::setConfigForNode(const std::string& nodeName, int enabled, float speed, const std::string& bezier, const std::string& style) {
    auto pConfig = m_mAnimationConfig[nodeName];
    if (!pConfig)
        return;

    *pConfig = {
        .overridden       = true,
        .internalBezier   = bezier,
        .internalStyle    = style,
        .internalSpeed    = speed,
        .internalEnabled  = enabled,
        .pValues          = pConfig,
        .pParentAnimation = pConfig->pParentAnimation, // keep the parent!
    };

    setAnimForChildren(pConfig);
}

SP<SAnimationPropertyConfig> CAnimationConfigTree::getConfig(const std::string& name) const {
    return m_mAnimationConfig.at(name);
}

const std::unordered_map<std::string, SP<SAnimationPropertyConfig>>& CAnimationConfigTree::getFullConfig() const {
    return m_mAnimationConfig;
}

void CAnimationConfigTree::setAnimForChildren(SP<SAnimationPropertyConfig> PANIM) {
    for (auto& [name, anim] : m_mAnimationConfig) {
        if (anim->pParentAnimation == PANIM && !anim->overridden) {
            // if a child isnt overridden, set the values of the parent
            anim->pValues = PANIM->pValues;

            setAnimForChildren(anim);
        }
    }
}
