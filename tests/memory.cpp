#include <hyprutils/memory/WeakPtr.hpp>
#include "shared.hpp"

using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer

int main(int argc, char** argv, char** envp) {
    SP<int> intPtr = makeShared<int>(10);

    int ret = 0;

    EXPECT(*intPtr, 10);
    EXPECT(intPtr.strongRef(), 1);

    WP<int> weak = intPtr;

    EXPECT(*intPtr, 10);
    EXPECT(intPtr.strongRef(), 1);
    EXPECT(*weak.get(), 10);
    EXPECT(weak.expired(), false);

    intPtr = {};

    EXPECT(weak.expired(), true);

    return ret;
}