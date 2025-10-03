#include <hyprutils/memory/UniquePtr.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <hyprutils/memory/Atomic.hpp>
#include "shared.hpp"
#include <chrono>
#include <print>
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

static int testAtomicImpl() {
    int ret = 0;

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
        EXPECT(shared, false);
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

        EXPECT(shared.strongRef(), 0);
        EXPECT(weak.valid(), false);

        auto shared2 = weak.lock();
        EXPECT(shared, false);
        EXPECT(shared2.get(), nullptr);
        EXPECT(shared.strongRef(), 0);
        EXPECT(weak.valid(), false);
        EXPECT(weak.expired(), true);
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

        EXPECT(!!genericAtomic, true);
        EXPECT(!!genericNormal, true);
    }

    return ret;
}

int main(int argc, char** argv, char** envp) {
    SP<int> intPtr    = makeShared<int>(10);
    SP<int> intPtr2   = makeShared<int>(-1337);
    UP<int> intUnique = makeUnique<int>(420);

    int     ret = 0;

    EXPECT(*intPtr, 10);
    EXPECT(intPtr.strongRef(), 1);
    EXPECT(*intUnique, 420);

    WP<int> weak       = intPtr;
    WP<int> weakUnique = intUnique;

    EXPECT(*intPtr, 10);
    EXPECT(intPtr.strongRef(), 1);
    EXPECT(*weak, 10);
    EXPECT(weak.expired(), false);
    EXPECT(*weakUnique, 420);
    EXPECT(weakUnique.expired(), false);
    EXPECT(intUnique.impl_->wref(), 1);

    SP<int> sharedFromUnique = weakUnique.lock();
    EXPECT(sharedFromUnique, nullptr);

    std::vector<SP<int>> sps;
    sps.push_back(intPtr);
    sps.emplace_back(intPtr);
    sps.push_back(intPtr2);
    sps.emplace_back(intPtr2);
    std::erase_if(sps, [intPtr](const auto& e) { return e == intPtr; });

    intPtr.reset();
    intUnique.reset();

    EXPECT(weak.impl_->ref(), 0);
    EXPECT(weakUnique.impl_->ref(), 0);
    EXPECT(weakUnique.impl_->wref(), 1);
    EXPECT(intPtr2.strongRef(), 3);

    EXPECT(weak.expired(), true);
    EXPECT(weakUnique.expired(), true);

    auto intPtr2AsUint = reinterpretPointerCast<unsigned int>(intPtr2);
    EXPECT(intPtr2.strongRef(), 4);
    EXPECT(intPtr2AsUint.strongRef(), 4);

    EXPECT(*intPtr2AsUint > 0, true);
    EXPECT(*intPtr2AsUint, (unsigned int)(int)-1337);
    *intPtr2AsUint = 10;
    EXPECT(*intPtr2AsUint, 10);
    EXPECT(*intPtr2, 10);

    EXPECT(testAtomicImpl(), 0);
    return ret;
}
