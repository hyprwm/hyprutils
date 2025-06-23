#include <any>
#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <memory>
#include <stdexcept>
#include "hyprutils/memory/SharedPtr.hpp"
#include "hyprutils/signal/Listener.hpp"
#include "shared.hpp"

using namespace Hyprutils::Signal;
using namespace Hyprutils::Memory;

void legacy(int& ret) {
    CSignal signal;
    int     data     = 0;
    auto    listener = signal.registerListener([&]([[maybe_unused]] std::any d) { data = 1; });

    signal.emit();

    EXPECT(data, 1);

    data = 0;

    listener.reset();

    signal.emit();

    EXPECT(data, 0);
}

void legacyListenerEmit(int& ret) {
    int     data = 0;
    CSignal signal;
    auto    listener = signal.registerListener([&](std::any d) { data = std::any_cast<int>(d); });

    listener->emit(1); // not a typo
    EXPECT(data, 1);
}

void empty(int& ret) {
    int        data = 0;

    CSignalT<> signal;
    auto       listener = signal.registerListener([&] { data = 1; });

    signal.emit();
    EXPECT(data, 1);

    data = 0;
    listener.reset();
    signal.emit();
    EXPECT(data, 0);
}

void typed(int& ret) {
    int           data = 0;

    CSignalT<int> signal;
    auto          listener = signal.registerListener([&](int newData) { data = newData; });

    signal.emit(1);
    EXPECT(data, 1);
}

void typedMany(int& ret) {
    int                     data1 = 0;
    int                     data2 = 0;
    int                     data3 = 0;

    CSignalT<int, int, int> signal;
    auto                    listener = signal.registerListener([&](int d1, int d2, int d3) {
        data1 = d1;
        data2 = d2;
        data3 = d3;
    });

    signal.emit(1, 2, 3);
    EXPECT(data1, 1);
    EXPECT(data2, 2);
    EXPECT(data3, 3);
}

void listenerAdded(int& ret) {
    int                 count = 0;

    CSignalT<>          signal;
    CHyprSignalListener secondListener;

    auto                listener = signal.registerListener([&] {
        count += 1;

        if (!secondListener)
            secondListener = signal.registerListener([&] { count += 1; });
    });

    signal.emit();
    EXPECT(count, 1); // second should NOT be invoked as it was registed during emit

    signal.emit();
    EXPECT(count, 3); // second should be invoked
}

void lastListenerSwapped(int& ret) {
    int                 count = 0;

    CSignalT<>          signal;
    CHyprSignalListener removedListener;
    CHyprSignalListener addedListener;

    auto                firstListener = signal.registerListener([&] {
        removedListener.reset(); // dropped and should NOT be invoked

        if (!addedListener)
            addedListener = signal.registerListener([&] { count += 2; });
    });

    removedListener = signal.registerListener([&] { count += 1; });

    signal.emit();
    EXPECT(count, 0); // neither the removed nor added listeners should fire

    signal.emit();
    EXPECT(count, 2); // only the new listener should fire
}

void signalDestroyed(int& ret) {
    int  count = 0;

    auto signal = std::make_unique<CSignalT<>>();

    // This ensures a destructor of a listener called before signal reset is safe.
    auto preListener = signal->registerListener([&] { count += 1; });

    auto listener = signal->registerListener([&] { signal.reset(); });

    // This ensures a destructor of a listener called after signal reset is safe
    // and gets called.
    auto postListener = signal->registerListener([&] { count += 1; });

    signal->emit();
    EXPECT(count, 2); // all listeners should fire regardless of signal deletion
}

void signalDestroyedWithAddedListener(int& ret) {
    int                 count = 0;

    auto                signal = std::make_unique<CSignalT<>>();
    CHyprSignalListener shouldNotRun;

    auto                listener = signal->registerListener([&] {
        shouldNotRun = signal->registerListener([&] { count += 2; });
        signal.reset();
    });

    signal->emit();
    EXPECT(count, 0);
}

void signalDestroyedWithRemovedAndAddedListener(int& ret) {
    int                 count = 0;

    auto                signal = std::make_unique<CSignalT<>>();
    CHyprSignalListener removed;
    CHyprSignalListener shouldNotRun;

    auto                listener = signal->registerListener([&] {
        removed.reset();
        shouldNotRun = signal->registerListener([&] { count += 2; });
        signal.reset();
    });

    removed = signal->registerListener([&] { count += 1; });

    signal->emit();
    EXPECT(count, 0);
}

void staticListener(int& ret) {
    struct STestOwner {
        int data = 0;
    } owner;

    CSignalT<int> signal;
    signal.registerStaticListener<STestOwner>([&](STestOwner* owner, int newData) { owner->data = newData; }, &owner);

    signal.emit(1);
    EXPECT(owner.data, 1);
}

void staticListenerDestroy(int& ret) {
    int  data = 0;

    auto signal = makeShared<CSignalT<>>();
    signal->registerStaticListener([&] { data += 1; });

    signal->registerStaticListener([&] {
        // should not fire but SHOULD be freed
        signal->registerStaticListener([&] { data += 3; });

        signal.reset();
    });

    signal->registerStaticListener([&] { data += 1; });

    signal->emit();
    EXPECT(data, 2);
}

void notReentrant(int& ret) {
    int count = 0;

    CSignalT<> signal;
    auto listener = signal.registerListener([&] { signal.emit(); });

    try {
        signal.emit();
    } catch(std::logic_error& e) {
        count += 1;
    }

    EXPECT(count, 1);
}

int main(int argc, char** argv, char** envp) {
    int ret = 0;
    legacy(ret);
    legacyListenerEmit(ret);
    empty(ret);
    typed(ret);
    typedMany(ret);
    listenerAdded(ret);
    lastListenerSwapped(ret);
    signalDestroyed(ret);
    signalDestroyedWithAddedListener(ret);
    signalDestroyedWithRemovedAndAddedListener(ret);
    staticListener(ret);
    staticListenerDestroy(ret);
    notReentrant(ret);
    return ret;
}
