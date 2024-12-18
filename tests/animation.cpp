#include <hyprutils/animation/AnimationManager.hpp>
#include <hyprutils/animation/AnimatedVariable.hpp>
#include "shared.hpp"

using namespace Hyprutils::Animation;
using namespace Hyprutils::Math;

class EmtpyContext {};

template <typename VarType>
using CAnimatedVariable = CGenericAnimatedVariable<VarType, EmtpyContext>;

enum eAVTypes {
    INT = 1,
    TEST,
};

struct SomeTestType {
    bool done = false;
    bool operator==(const SomeTestType& other) const {
        return done == other.done;
    }
    SomeTestType& operator=(const SomeTestType& other) {
        done = other.done;
        return *this;
    }
};

#define INITANIMCFG(name)           animationConfig[name] = {}
#define CREATEANIMCFG(name, parent) animationConfig[name] = {false, "", "", 0.f, -1, &animationConfig["global"], &animationConfig[parent]}

std::unordered_map<std::string, SAnimationPropertyConfig> animationConfig;

class CMyAnimationManager : public CAnimationManager {
  public:
    void tick() {
        for (auto const& av : m_vActiveAnimatedVariables) {
            if (!av->ok())
                continue;

            const auto SPENT   = av->getPercent();
            const auto PBEZIER = getBezier(av->getBezierName());
            const auto POINTY  = PBEZIER->getYForPoint(SPENT);

            if (POINTY >= 1.f || !av->enabled()) {
                av->warp();
                continue;
            }

            switch (av->m_Type) {
                case eAVTypes::INT: {
                    auto* avInt = dynamic_cast<CAnimatedVariable<int>*>(av);
                    if (!avInt)
                        std::cout << Colors::RED << "Dynamic cast upcast failed" << Colors::RESET;

                    const auto DELTA = avInt->goal() - avInt->value();
                    avInt->value()   = avInt->begun() + (DELTA * POINTY);
                } break;
                case eAVTypes::TEST: {
                    auto* avCustom = dynamic_cast<CAnimatedVariable<SomeTestType>*>(av);
                    if (!avCustom)
                        std::cout << Colors::RED << "Dynamic cast upcast failed" << Colors::RESET;

                    if (SPENT >= 1.f)
                        avCustom->value().done = true;
                } break;
                default: {
                    std::cout << Colors::RED << "What are we even doing?" << Colors::RESET;
                } break;
            }

            av->onUpdate();
        }

        tickDone();
    }

    template <typename AnimType>
    void addAnimation(const AnimType& v, CAnimatedVariable<AnimType>& av, const std::string& animationConfigName) {
        constexpr const eAVTypes EAVTYPE = std::is_same_v<AnimType, int> ? eAVTypes::INT : eAVTypes::TEST;
        av.create(EAVTYPE, v, static_cast<CAnimationManager*>(this));
        av.setConfig(&animationConfig[animationConfigName]);
    }

    virtual void scheduleTick() {
        ;
    }

    virtual void onTicked() {
        ;
    }
};

CMyAnimationManager gAnimationManager;

class Subject {
  public:
    Subject(const int& a, const int& b) {
        gAnimationManager.addAnimation(a, m_iA, "default");
        gAnimationManager.addAnimation(b, m_iB, "default");
        gAnimationManager.addAnimation({}, m_iC, "default");
    }
    CAnimatedVariable<int>          m_iA;
    CAnimatedVariable<int>          m_iB;
    CAnimatedVariable<SomeTestType> m_iC;
};

int main(int argc, char** argv, char** envp) {
    INITANIMCFG("global");
    CREATEANIMCFG("default", "global");
    animationConfig["default"].internalBezier  = "default";
    animationConfig["default"].internalSpeed   = 1.0;
    animationConfig["default"].internalEnabled = 1;
    animationConfig["default"].pValues         = &animationConfig["default"];

    int     ret = 0;
    Subject s(0, 0);

    EXPECT(s.m_iA.value(), 0);
    EXPECT(s.m_iB.value(), 0);

    // Test destruction of a CAnimatedVariable
    {
        Subject s2(10, 10);
        // Adds them to active
        s2.m_iA = 1;
        s2.m_iB = 2;
        // We deliberately do not tick here, to make sure the destructor removes active animated variables
    }

    EXPECT(gAnimationManager.shouldTickForNext(), false);
    EXPECT(s.m_iC.value().done, false);

    s.m_iA = 10;
    s.m_iB = 100;
    s.m_iC = SomeTestType(true);

    EXPECT(s.m_iC.value().done, false);

    while (gAnimationManager.shouldTickForNext()) {
        gAnimationManager.tick();
    }

    EXPECT(s.m_iA.value(), 10);
    EXPECT(s.m_iB.value(), 100);
    EXPECT(s.m_iC.value().done, true);

    s.m_iA.setValue(0);
    s.m_iB.setValue(0);

    while (gAnimationManager.shouldTickForNext()) {
        gAnimationManager.tick();
    }

    EXPECT(s.m_iA.value(), 10);
    EXPECT(s.m_iB.value(), 100);

    //
    // Test callbacks
    //
    bool beginCallbackRan  = false;
    bool updateCallbackRan = false;
    bool endCallbackRan    = false;
    s.m_iA.setCallbackOnBegin([&beginCallbackRan](CBaseAnimatedVariable* av) { beginCallbackRan = true; });
    s.m_iA.setUpdateCallback([&updateCallbackRan](CBaseAnimatedVariable* av) { updateCallbackRan = true; });
    s.m_iA.setCallbackOnEnd([&endCallbackRan](CBaseAnimatedVariable* av) { endCallbackRan = true; }, false);

    s.m_iA.setValueAndWarp(42);

    EXPECT(beginCallbackRan, false);
    EXPECT(updateCallbackRan, true);
    EXPECT(endCallbackRan, true);

    beginCallbackRan  = false;
    updateCallbackRan = false;
    endCallbackRan    = false;

    s.m_iA = 1337;
    while (gAnimationManager.shouldTickForNext()) {
        gAnimationManager.tick();
    }

    EXPECT(beginCallbackRan, true);
    EXPECT(updateCallbackRan, true);
    EXPECT(endCallbackRan, true);

    return ret;
}
