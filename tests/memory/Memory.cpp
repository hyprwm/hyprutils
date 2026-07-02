
#include <hyprutils/memory/Atomic.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>

using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

#define ASP        CAtomicSharedPointer
#define AWP        CAtomicWeakPointer
#define NTHREADS   8
#define ITERATIONS 10000

namespace {
    class Peepee {};
    class Poopoo {};
}

template <typename A, typename B>
concept EqualityComparable = requires(A a, B b) { a == b; };

static_assert(!EqualityComparable<SP<Peepee>, SP<Poopoo>>);
static_assert(!EqualityComparable<WP<Peepee>, WP<Poopoo>>);
static_assert(!EqualityComparable<UP<Peepee>, UP<Poopoo>>);
static_assert(!EqualityComparable<ASP<Peepee>, ASP<Poopoo>>);
static_assert(!EqualityComparable<AWP<Peepee>, AWP<Poopoo>>);

static void testAtomicImpl() {
    {
        // Using makeShared here could lead to invalid refcounts.
        ASP<int>                 shared = makeAtomicShared<int>(0);
        std::vector<std::thread> threads;

        threads.reserve(NTHREADS);
        for (size_t i = 0; i < NTHREADS; i++) {
            threads.emplace_back([shared]() {
                for (size_t j = 0; j < ITERATIONS; j++) {
                    ASP<int> strongRef = shared;
                    (*shared)++;
                    strongRef.reset();
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // Actual count is not incremented in a thread-safe manner here, so we can't check it.
        // We just want to check that the concurent refcounting doesn't cause any memory corruption.
        shared.reset();
        EXPECT_FALSE(shared);
    }

    {
        ASP<int>                 shared = makeAtomicShared<int>(0);
        AWP<int>                 weak   = shared;
        std::vector<std::thread> threads;

        threads.reserve(NTHREADS);
        for (size_t i = 0; i < NTHREADS; i++) {
            threads.emplace_back([weak]() {
                for (size_t j = 0; j < ITERATIONS; j++) {
                    if (auto s = weak.lock(); s) {
                        (*s)++;
                    }
                }
            });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        shared.reset();

        for (auto& thread : threads) {
            thread.join();
        }

        EXPECT_EQ(shared.strongRef(), 0);
        EXPECT_EQ(weak.valid(), false);

        auto shared2 = weak.lock();
        EXPECT_FALSE(shared);
        EXPECT_EQ(shared2.get(), nullptr);
        EXPECT_EQ(shared.strongRef(), 0);
        EXPECT_EQ(weak.valid(), false);
        EXPECT_EQ(weak.expired(), true);
    }

    { // This tests recursive deletion. When foo will be deleted, bar will be deleted within the foo dtor.
        class CFoo {
          public:
            AWP<CFoo> bar;
        };

        ASP<CFoo> foo = makeAtomicShared<CFoo>();
        foo->bar      = foo;
    }

    { // This tests destroying the data when storing the base class of a type
        class ITest {
          public:
            size_t num = 0;
            ITest() : num(1234) {};
        };

        class CA : public ITest {
          public:
            size_t num2 = 0;
            CA() : ITest(), num2(4321) {};
        };

        class CB : public ITest {
          public:
            int num2 = 0;
            CB() : ITest(), num2(-1) {};
        };

        ASP<ITest> genericAtomic = nullptr;
        SP<ITest>  genericNormal = nullptr;
        {
            auto derivedAtomic = makeAtomicShared<CA>();
            auto derivedNormal = makeShared<CA>();
            genericAtomic      = derivedAtomic;
            genericNormal      = derivedNormal;
        }

        EXPECT_EQ(!!genericAtomic, true);
        EXPECT_EQ(!!genericNormal, true);
    }
}

class InterfaceA {
  public:
    virtual ~InterfaceA() = default;
    int m_ifaceAInt       = 69;
    int m_ifaceAShit      = 1;
};

class InterfaceB {
  public:
    virtual ~InterfaceB() = default;
    int m_ifaceBInt       = 2;
    int m_ifaceBShit      = 3;
};

class CChild : public InterfaceA, public InterfaceB {
  public:
    virtual ~CChild() = default;
    int m_childInt    = 4;
};

class CChildA : public InterfaceA {
  public:
    int m_childAInt = 4;
};

class IVirtualRoot {
  public:
    virtual ~IVirtualRoot() = default;
    int m_rootInt          = 5;
};

class IVirtualLeft : public virtual IVirtualRoot {
  public:
    virtual ~IVirtualLeft() = default;
    int m_leftInt          = 6;
};

class IVirtualRight : public virtual IVirtualRoot {
  public:
    virtual ~IVirtualRight() = default;
    int m_rightInt          = 7;
};

class CVirtualChild : public IVirtualLeft, public IVirtualRight {
  public:
    virtual ~CVirtualChild() = default;
    int m_virtualChildInt   = 8;
};

class CConstObject {
  public:
    int m_value = 9;
};

static void testHierarchy() {
    // Same test for atomic and non-atomic
    {
        SP<CChildA> childA = makeShared<CChildA>();
        auto        ifaceA = SP<InterfaceA>(childA);
        EXPECT_TRUE(ifaceA);
        EXPECT_EQ(ifaceA->m_ifaceAInt, 69);

        auto ifaceB  = dynamicPointerCast<InterfaceA>(SP<CChildA>{});
        auto ifaceB2 = dynamicPointerCast<InterfaceA>(WP<CChildA>{});
        EXPECT_TRUE(!ifaceB);
        EXPECT_TRUE(!ifaceB2);
    }

    {
        SP<CChild>     child  = makeShared<CChild>();
        SP<InterfaceA> ifaceA = dynamicPointerCast<InterfaceA>(child);
        SP<InterfaceB> ifaceB = dynamicPointerCast<InterfaceB>(child);
        EXPECT_TRUE(ifaceA);
        EXPECT_TRUE(ifaceB);

        EXPECT_EQ(ifaceA->m_ifaceAInt, 69);
        EXPECT_EQ(ifaceB->m_ifaceBInt, 2);

        WP<InterfaceA> ifaceAWeak = ifaceA;
        WP<InterfaceB> ifaceBWeak = dynamicPointerCast<InterfaceB>(WP<CChild>{child});

        child.reset();
        EXPECT_TRUE(ifaceAWeak);
        EXPECT_TRUE(ifaceBWeak);
        EXPECT_TRUE(ifaceA);
        EXPECT_EQ(ifaceAWeak->m_ifaceAInt, 69);
        EXPECT_EQ(ifaceA->m_ifaceAInt, 69);
        ifaceA.reset();
        EXPECT_TRUE(ifaceAWeak);
        EXPECT_EQ(ifaceAWeak->m_ifaceAInt, 69);
        EXPECT_TRUE(ifaceB);
        EXPECT_EQ(ifaceB->m_ifaceBInt, 2);
        EXPECT_EQ(ifaceBWeak->m_ifaceBInt, 2);
        ifaceB.reset();
        EXPECT_TRUE(!ifaceAWeak);
        EXPECT_TRUE(!ifaceBWeak);
    }

    //

    {
        ASP<CChildA> childA = makeAtomicShared<CChildA>();
        auto         ifaceA = ASP<InterfaceA>(childA);
        EXPECT_TRUE(ifaceA);
        EXPECT_EQ(ifaceA->m_ifaceAInt, 69);

        auto ifaceB  = dynamicPointerCast<InterfaceA>(SP<CChildA>{});
        auto ifaceB2 = dynamicPointerCast<InterfaceA>(WP<CChildA>{});
        EXPECT_TRUE(!ifaceB);
        EXPECT_TRUE(!ifaceB2);
    }

    {
        ASP<CChild>     child  = makeAtomicShared<CChild>();
        ASP<InterfaceA> ifaceA = dynamicPointerCast<InterfaceA>(child);
        ASP<InterfaceB> ifaceB = dynamicPointerCast<InterfaceB>(child);
        EXPECT_TRUE(ifaceA);
        EXPECT_TRUE(ifaceB);

        EXPECT_EQ(ifaceA->m_ifaceAInt, 69);
        EXPECT_EQ(ifaceB->m_ifaceBInt, 2);

        AWP<InterfaceA> ifaceAWeak = ifaceA;
        AWP<InterfaceB> ifaceBWeak = dynamicPointerCast<InterfaceB>(AWP<CChild>{child});

        child.reset();
        EXPECT_TRUE(ifaceAWeak);
        EXPECT_TRUE(ifaceBWeak);
        EXPECT_TRUE(ifaceA);
        EXPECT_EQ(ifaceAWeak->m_ifaceAInt, 69);
        EXPECT_EQ(ifaceA->m_ifaceAInt, 69);
        ifaceA.reset();
        EXPECT_TRUE(ifaceAWeak);
        EXPECT_EQ(ifaceAWeak->m_ifaceAInt, 69);
        EXPECT_TRUE(ifaceB);
        EXPECT_EQ(ifaceB->m_ifaceBInt, 2);
        EXPECT_EQ(ifaceBWeak->m_ifaceBInt, 2);
        ifaceB.reset();
        EXPECT_TRUE(!ifaceAWeak);
        EXPECT_TRUE(!ifaceBWeak);
    }

    // test for leaks
    for (size_t i = 0; i < 10000; ++i) {
        auto child  = makeAtomicShared<CChild>();
        auto child2 = makeShared<CChild>();
    }
}

static void testVirtualHierarchy() {
    {
        SP<CVirtualChild> child = makeShared<CVirtualChild>();

        SP<IVirtualRoot>  root  = child;
        SP<IVirtualRight> right = dynamicPointerCast<IVirtualRight>(root);
        SP<CVirtualChild> recastChild = dynamicPointerCast<CVirtualChild>(root);

        EXPECT_TRUE(root);
        EXPECT_TRUE(right);
        EXPECT_TRUE(recastChild);
        EXPECT_EQ(root->m_rootInt, 5);
        EXPECT_EQ(right->m_rightInt, 7);
        EXPECT_EQ(recastChild->m_virtualChildInt, 8);

        SP<IVirtualRoot> rootFromCast = dynamicPointerCast<IVirtualRoot>(child);
        EXPECT_TRUE(rootFromCast);
        EXPECT_EQ(rootFromCast->m_rootInt, 5);

        WP<IVirtualRoot>  rootWeak  = child;
        WP<IVirtualRight> rightWeak = dynamicPointerCast<IVirtualRight>(rootWeak);

        EXPECT_TRUE(rootWeak);
        EXPECT_TRUE(rightWeak);
        EXPECT_EQ(rootWeak->m_rootInt, 5);
        EXPECT_EQ(rightWeak->m_rightInt, 7);
        EXPECT_EQ(rightWeak.lock()->m_rightInt, 7);
    }

    {
        ASP<CVirtualChild> child = makeAtomicShared<CVirtualChild>();

        ASP<IVirtualRoot>  root  = child;
        ASP<IVirtualRight> right = dynamicPointerCast<IVirtualRight>(root);
        ASP<CVirtualChild> recastChild = dynamicPointerCast<CVirtualChild>(root);

        EXPECT_TRUE(root);
        EXPECT_TRUE(right);
        EXPECT_TRUE(recastChild);
        EXPECT_EQ(root->m_rootInt, 5);
        EXPECT_EQ(right->m_rightInt, 7);
        EXPECT_EQ(recastChild->m_virtualChildInt, 8);

        ASP<IVirtualRoot> rootFromCast = dynamicPointerCast<IVirtualRoot>(child);
        EXPECT_TRUE(rootFromCast);
        EXPECT_EQ(rootFromCast->m_rootInt, 5);

        AWP<IVirtualRoot>  rootWeak  = root;
        AWP<IVirtualRight> rightWeak = dynamicPointerCast<IVirtualRight>(rootWeak);

        EXPECT_TRUE(rootWeak);
        EXPECT_TRUE(rightWeak);
        EXPECT_EQ(rootWeak->m_rootInt, 5);
        EXPECT_EQ(rightWeak->m_rightInt, 7);
        EXPECT_EQ(rightWeak.lock()->m_rightInt, 7);
    }
}

static void testConstPointers() {
    {
        SP<const CConstObject> shared = makeShared<const CConstObject>();
        WP<const CConstObject> weak   = shared;

        EXPECT_TRUE(shared);
        EXPECT_TRUE(weak);
        EXPECT_EQ(shared->m_value, 9);
        EXPECT_EQ(weak->m_value, 9);
    }

    {
        UP<const CConstObject> unique = makeUnique<const CConstObject>();
        WP<const CConstObject> weak   = unique;

        EXPECT_TRUE(unique);
        EXPECT_TRUE(weak);
        EXPECT_EQ(unique->m_value, 9);
        EXPECT_EQ(weak->m_value, 9);
    }

    {
        SP<const CVirtualChild> child = makeShared<const CVirtualChild>();

        SP<const IVirtualRoot>  root  = child;
        SP<const IVirtualRight> right = dynamicPointerCast<const IVirtualRight>(root);
        SP<const CVirtualChild> recastChild = dynamicPointerCast<const CVirtualChild>(root);

        EXPECT_TRUE(root);
        EXPECT_TRUE(right);
        EXPECT_TRUE(recastChild);
        EXPECT_EQ(root->m_rootInt, 5);
        EXPECT_EQ(right->m_rightInt, 7);
        EXPECT_EQ(recastChild->m_virtualChildInt, 8);

        WP<const IVirtualRoot>  rootWeak  = child;
        WP<const IVirtualRight> rightWeak = dynamicPointerCast<const IVirtualRight>(rootWeak);

        EXPECT_TRUE(rootWeak);
        EXPECT_TRUE(rightWeak);
        EXPECT_EQ(rootWeak->m_rootInt, 5);
        EXPECT_EQ(rightWeak->m_rightInt, 7);
    }

    {
        ASP<const CVirtualChild> child = makeAtomicShared<const CVirtualChild>();

        ASP<const IVirtualRoot>  root  = child;
        ASP<const IVirtualRight> right = dynamicPointerCast<const IVirtualRight>(root);
        ASP<const CVirtualChild> recastChild = dynamicPointerCast<const CVirtualChild>(root);

        EXPECT_TRUE(root);
        EXPECT_TRUE(right);
        EXPECT_TRUE(recastChild);
        EXPECT_EQ(root->m_rootInt, 5);
        EXPECT_EQ(right->m_rightInt, 7);
        EXPECT_EQ(recastChild->m_virtualChildInt, 8);

        AWP<const IVirtualRoot>  rootWeak  = root;
        AWP<const IVirtualRight> rightWeak = dynamicPointerCast<const IVirtualRight>(rootWeak);

        EXPECT_TRUE(rootWeak);
        EXPECT_TRUE(rightWeak);
        EXPECT_EQ(rootWeak->m_rootInt, 5);
        EXPECT_EQ(rightWeak->m_rightInt, 7);
    }
}

class CSelfDestruct {
  public:
    SP<CSelfDestruct> self;

    //
    void youShouldKysNOW() {
        self.reset();
    }
};

static void testSelfDestruct() {
    auto x                 = makeShared<CSelfDestruct>();
    x->self                = x;
    WP<CSelfDestruct> weak = x;
    x.reset();

    // this has no EXPECT, because all we check is if there isn't a UAF here.
    // if there is, asan will abort us
    weak->youShouldKysNOW();
}

TEST(Memory, memory) {
    SP<int> intPtr    = makeShared<int>(10);
    SP<int> intPtr2   = makeShared<int>(-1337);
    UP<int> intUnique = makeUnique<int>(420);

    EXPECT_EQ(*intPtr, 10);
    EXPECT_EQ(intPtr.strongRef(), 1);
    EXPECT_EQ(*intUnique, 420);

    WP<int> weak       = intPtr;
    WP<int> weakUnique = intUnique;

    EXPECT_EQ(*intPtr, 10);
    EXPECT_EQ(intPtr.strongRef(), 1);
    EXPECT_EQ(*weak, 10);
    EXPECT_EQ(weak.expired(), false);
    EXPECT_EQ(!!weak.lock(), true);
    EXPECT_EQ(*weakUnique, 420);
    EXPECT_EQ(weakUnique.expired(), false);
    EXPECT_EQ(intUnique.impl_->wref(), 1);

    SP<int> sharedFromUnique = weakUnique.lock();
    EXPECT_EQ(sharedFromUnique, nullptr);

    std::vector<SP<int>> sps;
    sps.push_back(intPtr);
    sps.emplace_back(intPtr);
    sps.push_back(intPtr2);
    sps.emplace_back(intPtr2);
    std::erase_if(sps, [intPtr](const auto& e) { return e == intPtr; });

    intPtr.reset();
    intUnique.reset();

    EXPECT_EQ(weak.impl_->ref(), 0);
    EXPECT_EQ(weakUnique.impl_->ref(), 0);
    EXPECT_EQ(weakUnique.impl_->wref(), 1);
    EXPECT_EQ(intPtr2.strongRef(), 3);

    EXPECT_EQ(weak.expired(), true);
    EXPECT_EQ(weakUnique.expired(), true);

    auto intPtr2AsUint = reinterpretPointerCast<unsigned int>(intPtr2);
    EXPECT_EQ(intPtr2.strongRef(), 4);
    EXPECT_EQ(intPtr2AsUint.strongRef(), 4);

    EXPECT_EQ(*intPtr2AsUint > 0, true);
    EXPECT_EQ(*intPtr2AsUint, (unsigned int)(int)-1337);
    *intPtr2AsUint = 10;
    EXPECT_EQ(*intPtr2AsUint, 10);
    EXPECT_EQ(*intPtr2, 10);

    testAtomicImpl();

    testHierarchy();

    testVirtualHierarchy();

    testConstPointers();

    testSelfDestruct();
}
