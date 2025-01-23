#include <hyprutils/memory/WeakPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include "shared.hpp"
#include <vector>

using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

int main(int argc, char** argv, char** envp) {
    SP<int> intPtr    = makeShared<int>(10);
    SP<int> intPtr2   = makeShared<int>(1337);
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

    return ret;
}