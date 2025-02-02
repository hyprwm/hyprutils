#pragma once

#include "../memory/WeakPtr.hpp"

#include <string>
#include <unordered_map>

namespace Hyprutils {
    namespace Animation {
        /*
            Structure for animation properties.
            Config properties need to have a static lifetime to allow for config reload.
        */
        struct SAnimationPropertyConfig {
            bool                                           overridden = false;

            std::string                                    internalBezier  = "";
            std::string                                    internalStyle   = "";
            float                                          internalSpeed   = 0.f;
            int                                            internalEnabled = -1;

            Memory::CWeakPointer<SAnimationPropertyConfig> pValues;
            Memory::CWeakPointer<SAnimationPropertyConfig> pParentAnimation;
        };

        /* A class to manage SAnimationPropertyConfig objects in a tree structure */
        class CAnimationConfigTree {
          public:
            CAnimationConfigTree()  = default;
            ~CAnimationConfigTree() = default;

            /* Add a new animation node inheriting from a parent.
               If parent is empty, a root node will be created that references it's own values.
               Make sure the parent node has already been created through this interface. */
            void createNode(const std::string& nodeName, const std::string& parent = "");

            /* check if a node name has been created using createNode */
            bool nodeExists(const std::string& nodeName) const;

            /* Override the values of a node. The root node can also be overriden. */
            void setConfigForNode(const std::string& nodeName, int enabled, float speed, const std::string& bezier, const std::string& style = "");

            Memory::CSharedPointer<SAnimationPropertyConfig>                                         getConfig(const std::string& name) const;
            const std::unordered_map<std::string, Memory::CSharedPointer<SAnimationPropertyConfig>>& getFullConfig() const;

            CAnimationConfigTree(const CAnimationConfigTree&)            = delete;
            CAnimationConfigTree(CAnimationConfigTree&&)                 = delete;
            CAnimationConfigTree& operator=(const CAnimationConfigTree&) = delete;
            CAnimationConfigTree& operator=(CAnimationConfigTree&&)      = delete;

          private:
            void                                                                              setAnimForChildren(Memory::CSharedPointer<SAnimationPropertyConfig> PANIM);
            std::unordered_map<std::string, Memory::CSharedPointer<SAnimationPropertyConfig>> m_mAnimationConfig;
        };
    }
}
