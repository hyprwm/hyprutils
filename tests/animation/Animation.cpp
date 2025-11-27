
#include <gtest/gtest.h>

#include <hyprutils/animation/AnimationConfig.hpp>
#include <hyprutils/animation/AnimationManager.hpp>
#include <hyprutils/animation/AnimatedVariable.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

using namespace Hyprutils::Animation;
using namespace Hyprutils::Math;
using namespace Hyprutils::Memory;

class EmtpyContext {};

template <typename VarType>
using CAnimatedVariable = CGenericAnimatedVariable<VarType, EmtpyContext>;

template <typename VarType>
using PANIMVAR = UP<CAnimatedVariable<VarType>>;

template <typename VarType>
using PANIMVARREF = WP<CAnimatedVariable<VarType>>;

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

CAnimationConfigTree animationTree;

class CMyAnimationManager : public CAnimationManager {
  public:
    void tick() {
        for (size_t i = 0; i < m_vActiveAnimatedVariables.size(); i++) {
            const auto PAV = m_vActiveAnimatedVariables[i];
            if (!PAV || !PAV->ok() || !PAV->isBeingAnimated())
                continue;

            const auto SPENT   = PAV->getPercent();
            const auto PBEZIER = getBezier(PAV->getBezierName());

            if (SPENT >= 1.f || !PAV->enabled()) {
                PAV->warp(true, false);
                continue;
            }

            const auto POINTY = PBEZIER->getYForPoint(SPENT);

            switch (PAV->m_Type) {
                case eAVTypes::INT: {
                    auto avInt = dc<CAnimatedVariable<int>*>(PAV.get());
                    if (!avInt)
                        std::cout << "Dynamic cast upcast failed\n";

                    const auto DELTA = avInt->goal() - avInt->value();
                    avInt->value()   = avInt->begun() + (DELTA * POINTY);
                } break;
                case eAVTypes::TEST: {
                    auto avCustom = dc<CAnimatedVariable<SomeTestType>*>(PAV.get());
                    if (!avCustom)
                        std::cout << "Dynamic cast upcast failed\n";

                    if (SPENT >= 1.f)
                        avCustom->value().done = true;
                } break;
                default: {
                    std::cout << "What are we even doing?\n";
                } break;
            }

            PAV->onUpdate();
        }

        tickDone();
    }

    template <typename VarType>
    void createAnimation(const VarType& v, PANIMVAR<VarType>& av, const std::string& animationConfigName) {
        constexpr const eAVTypes EAVTYPE = std::is_same_v<VarType, int> ? eAVTypes::INT : eAVTypes::TEST;
        av     = makeUnique<CGenericAnimatedVariable<VarType, EmtpyContext>>();

        av->create(EAVTYPE, sc<CAnimationManager*>(this), av, v);
        av->setConfig(animationTree.getConfig(animationConfigName));
    }

    virtual void scheduleTick() {
        ;
    }

    virtual void onTicked() {
        ;
    }
};

UP<CMyAnimationManager> pAnimationManager;

class Subject {
  public:
    Subject(const int& a, const int& b) {
        pAnimationManager->createAnimation(a, m_iA, "default");
        pAnimationManager->createAnimation(b, m_iB, "internal");
        pAnimationManager->createAnimation({}, m_iC, "default");
    }
    PANIMVAR<int>          m_iA;
    PANIMVAR<int>          m_iB;
    PANIMVAR<SomeTestType> m_iC;
};

static int config() {
    pAnimationManager = makeUnique<CMyAnimationManager>();

    int ret = 0;

    animationTree.createNode("global");
    animationTree.createNode("internal");

    animationTree.createNode("foo", "internal");
    animationTree.createNode("default", "global");
    animationTree.createNode("bar", "default");

    /*
      internal
        ↳ foo
      global
        ↳ default
          ↳ bar
    */

    auto barCfg      = animationTree.getConfig("bar");
    auto internalCfg = animationTree.getConfig("internal");

    // internal is a root node and should point to itself
    EXPECT_EQ(internalCfg->pParentAnimation.get(), internalCfg.get());
    EXPECT_EQ(internalCfg->pValues.get(), internalCfg.get());

    animationTree.setConfigForNode("global", 1, 4.0, "default", "asdf");

    EXPECT_EQ(barCfg->internalEnabled, -1);
    {
        const auto PVALUES = barCfg->pValues.lock();
        EXPECT_EQ(PVALUES->internalEnabled, 1);
        EXPECT_EQ(PVALUES->internalBezier, "default");
        EXPECT_EQ(PVALUES->internalStyle, "asdf");
        EXPECT_EQ(PVALUES->internalSpeed, 4.0);
    }
    EXPECT_EQ(barCfg->pParentAnimation.get(), animationTree.getConfig("default").get());

    // Overwrite our own values
    animationTree.setConfigForNode("bar", 1, 4.2, "test", "qwer");

    {
        const auto PVALUES = barCfg->pValues.lock();
        EXPECT_EQ(PVALUES->internalEnabled, 1);
        EXPECT_EQ(PVALUES->internalBezier, "test");
        EXPECT_EQ(PVALUES->internalStyle, "qwer");
        EXPECT_EQ(PVALUES->internalSpeed, 4.2f);
    }

    // Now overwrite the parent
    animationTree.setConfigForNode("default", 0, 0.0, "zxcv", "foo");

    {
        // Expecting no change
        const auto PVALUES = barCfg->pValues.lock();
        EXPECT_EQ(PVALUES->internalEnabled, 1);
        EXPECT_EQ(PVALUES->internalBezier, "test");
        EXPECT_EQ(PVALUES->internalStyle, "qwer");
        EXPECT_EQ(PVALUES->internalSpeed, 4.2f);
    }

    return ret;
}

TEST(Animation, animation) {
    config();

    animationTree.createNode("global");
    animationTree.createNode("internal");

    animationTree.createNode("default", "global");
    animationTree.setConfigForNode("global", 1, 4.0, "default", "asdf");

    Subject s(0, 0);

    EXPECT_EQ(s.m_iA->value(), 0);
    EXPECT_EQ(s.m_iB->value(), 0);

    // Test destruction of a CAnimatedVariable
    {
        Subject s2(10, 10);
        // Adds them to active
        *s2.m_iA = 1;
        *s2.m_iB = 2;
        // We deliberately do not tick here, to make sure the destructor removes active animated variables
    }

    EXPECT_EQ(pAnimationManager->shouldTickForNext(), false);
    EXPECT_EQ(s.m_iC->value().done, false);

    *s.m_iA = 10;
    *s.m_iB = 100;
    *s.m_iC = SomeTestType(true);

    EXPECT_EQ(s.m_iC->value().done, false);

    while (pAnimationManager->shouldTickForNext()) {
        pAnimationManager->tick();
    }

    EXPECT_EQ(s.m_iA->value(), 10);
    EXPECT_EQ(s.m_iB->value(), 100);
    EXPECT_EQ(s.m_iC->value().done, true);

    s.m_iA->setValue(0);
    s.m_iB->setValue(0);

    while (pAnimationManager->shouldTickForNext()) {
        pAnimationManager->tick();
    }

    EXPECT_EQ(s.m_iA->value(), 10);
    EXPECT_EQ(s.m_iB->value(), 100);

    // Test config stuff
    EXPECT_EQ(s.m_iA->getBezierName(), "default");
    EXPECT_EQ(s.m_iA->getStyle(), "asdf");
    EXPECT_EQ(s.m_iA->enabled(), true);

    animationTree.getConfig("global")->internalEnabled = 0;

    EXPECT_EQ(s.m_iA->enabled(), false);

    *s.m_iA = 50;
    pAnimationManager->tick(); // Expecting a warp
    EXPECT_EQ(s.m_iA->value(), 50);

    // Test missing pValues
    animationTree.getConfig("global")->internalEnabled = 0;
    animationTree.getConfig("default")->pValues.reset();

    EXPECT_EQ(s.m_iA->enabled(), false);
    EXPECT_EQ(s.m_iA->getBezierName(), "default");
    EXPECT_EQ(s.m_iA->getStyle(), "");
    EXPECT_EQ(s.m_iA->getPercent(), 1.f);

    // Reset
    animationTree.setConfigForNode("default", 1, 1, "default");

    //
    // Test callbacks
    //
    int beginCallbackRan  = 0;
    int updateCallbackRan = 0;
    int endCallbackRan    = 0;
    s.m_iA->setCallbackOnBegin([&beginCallbackRan](WP<CBaseAnimatedVariable> pav) { beginCallbackRan++; });
    s.m_iA->setUpdateCallback([&updateCallbackRan](WP<CBaseAnimatedVariable> pav) { updateCallbackRan++; });
    s.m_iA->setCallbackOnEnd([&endCallbackRan](WP<CBaseAnimatedVariable> pav) { endCallbackRan++; }, false);

    s.m_iA->setValueAndWarp(42);

    EXPECT_EQ(beginCallbackRan, 0);
    EXPECT_EQ(updateCallbackRan, 1);
    EXPECT_EQ(endCallbackRan, 2); // first called when setting the callback, then when warping.

    *s.m_iA = 1337;
    while (pAnimationManager->shouldTickForNext()) {
        pAnimationManager->tick();
    }

    EXPECT_EQ(beginCallbackRan, 1);
    EXPECT_EQ(updateCallbackRan > 2, true);
    EXPECT_EQ(endCallbackRan, 3);

    std::vector<PANIMVAR<int>> vars;
    for (int i = 0; i < 10; i++) {
        vars.resize(vars.size() + 1);
        pAnimationManager->createAnimation(1, vars.back(), "default");
        *vars.back() = 1337;
    }

    // test adding / removing vars during a tick
    s.m_iA->resetAllCallbacks();
    s.m_iA->setUpdateCallback([&vars](WP<CBaseAnimatedVariable> v) {
        if (v.get() != vars.back().get())
            vars.back()->warp();
    });
    s.m_iA->setCallbackOnEnd([&s, &vars](auto) {
        vars.resize(vars.size() + 1);
        pAnimationManager->createAnimation(1, vars.back(), "default");
        *vars.back() = 1337;
    });

    *s.m_iA = 1000000;

    while (pAnimationManager->shouldTickForNext()) {
        pAnimationManager->tick();
    }

    EXPECT_EQ(s.m_iA->value(), 1000000);
    // all vars should be set to 1337
    EXPECT_EQ(std::find_if(vars.begin(), vars.end(), [](const auto& v) { return v->value() != 1337; }) == vars.end(), true);

    // test one-time callbacks
    s.m_iA->resetAllCallbacks();
    s.m_iA->setCallbackOnEnd([&endCallbackRan](auto) { endCallbackRan++; }, true);

    EXPECT_EQ(endCallbackRan, 4);

    s.m_iA->setValueAndWarp(10);

    EXPECT_EQ(endCallbackRan, 4);
    EXPECT_EQ(s.m_iA->value(), 10);

    // test warp
    *s.m_iA = 3;
    s.m_iA->setCallbackOnEnd([&endCallbackRan](auto) { endCallbackRan++; }, false);

    s.m_iA->warp(false);
    EXPECT_EQ(endCallbackRan, 4);

    *s.m_iA = 4;
    s.m_iA->warp(true);
    EXPECT_EQ(endCallbackRan, 5);

    // test getCurveValue
    *s.m_iA = 0;
    EXPECT_EQ(s.m_iA->getCurveValue(), 0.f);
    s.m_iA->warp();
    EXPECT_EQ(s.m_iA->getCurveValue(), 1.f);
    EXPECT_EQ(endCallbackRan, 6);

    // test end callback readding the var
    *s.m_iA = 5;
    s.m_iA->setCallbackOnEnd([&endCallbackRan](WP<CBaseAnimatedVariable> v) {
        endCallbackRan++;
        const auto PAV = dc<CAnimatedVariable<int>*>(v.get());

        *PAV = 10;
        PAV->setCallbackOnEnd([&endCallbackRan](WP<CBaseAnimatedVariable> v) { endCallbackRan++; });
    });

    while (pAnimationManager->shouldTickForNext()) {
        pAnimationManager->tick();
    }

    EXPECT_EQ(endCallbackRan, 8);
    EXPECT_EQ(s.m_iA->value(), 10);

    // Test duplicate active anim vars are not allowed
    {
        EXPECT_EQ(pAnimationManager->m_vActiveAnimatedVariables.size(), 0);
        PANIMVAR<int> a;
        pAnimationManager->createAnimation(1, a, "default");
        EXPECT_EQ(pAnimationManager->m_vActiveAnimatedVariables.size(), 0);
        *a = 10;
        EXPECT_EQ(pAnimationManager->m_vActiveAnimatedVariables.size(), 1);
        *a = 20;
        EXPECT_EQ(pAnimationManager->m_vActiveAnimatedVariables.size(), 1);
        a->warp();
        EXPECT_EQ(pAnimationManager->m_vActiveAnimatedVariables.size(), 0);
        EXPECT_EQ(a->value(), 20);
    }

    // Test no crash when animation manager gets destroyed
    {
        PANIMVAR<int> a;
        pAnimationManager->createAnimation(1, a, "default");
        *a = 10;
        pAnimationManager.reset();
        EXPECT_EQ(a->isAnimationManagerDead(), true);
        a->setValueAndWarp(11);
        EXPECT_EQ(a->value(), 11);
        *a = 12;
        a->warp();
        EXPECT_EQ(a->value(), 12);
        *a = 13;
    } // a gets destroyed

    EXPECT_EQ(pAnimationManager.get(), nullptr);
}
