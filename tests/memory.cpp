#include <hyprutils/memory/WeakPtr.hpp>
#include "shared.hpp"
#include <vector>

using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer

int main(int argc, char** argv, char** envp) {
    SP<int> intPtr  = makeShared<int>(10);
    SP<int> intPtr2 = makeShared<int>(1337);

    int     ret = 0;

    EXPECT(*intPtr, 10);
    EXPECT(intPtr.strongRef(), 1);

    WP<int> weak = intPtr;

    EXPECT(*intPtr, 10);
    EXPECT(intPtr.strongRef(), 1);
    EXPECT(*weak.get(), 10);
    EXPECT(weak.expired(), false);

    std::vector<SP<int>> sps;
    sps.push_back(intPtr);
    sps.emplace_back(intPtr);
    sps.push_back(intPtr2);
    sps.emplace_back(intPtr2);
    std::erase_if(sps, [intPtr](const auto& e) { return e == intPtr; });

    intPtr.reset();

    EXPECT(weak.impl_->ref(), 0);
    EXPECT(intPtr2.strongRef(), 3);

    EXPECT(weak.expired(), true);

    return ret;
}