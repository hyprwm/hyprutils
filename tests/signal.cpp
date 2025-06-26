#include <any>
#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <memory>
#include "hyprutils/memory/SharedPtr.hpp"
#include "hyprutils/signal/Listener.hpp"
#include "shared.hpp"

using namespace Hyprutils::Signal;
using namespace Hyprutils::Memory;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
//

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

void legacyListeners(int& ret) {
    int           data = 0;

    CSignalT<>    signal0;
    CSignalT<int> signal1;

    auto          listener0 = signal0.registerListener([&](std::any d) { data += 1; });
    auto          listener1 = signal1.registerListener([&](std::any d) { data += std::any_cast<int>(d); });

    signal0.registerStaticListener([&](void* o, std::any d) { data += 10; }, nullptr);
    signal1.registerStaticListener([&](void* o, std::any d) { data += std::any_cast<int>(d) * 10; }, nullptr);

    signal0.emit();
    signal1.emit(2);

    EXPECT(data, 33);
}

#pragma GCC diagnostic pop
//

void empty(int& ret) {
    int        data = 0;

    CSignalT<> signal;
    auto       listener = signal.listen([&] { data = 1; });

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
    auto          listener = signal.listen([&](int newData) { data = newData; });

    signal.emit(1);
    EXPECT(data, 1);
}

void ignoreParams(int& ret) {
    int           data = 0;

    CSignalT<int> signal;
    auto          listener = signal.listen([&] { data += 1; });

    signal.listenStatic([&] { data += 1; });

    signal.emit(2);
    EXPECT(data, 2);
}

void typedMany(int& ret) {
    int                     data1 = 0;
    int                     data2 = 0;
    int                     data3 = 0;

    CSignalT<int, int, int> signal;
    auto                    listener = signal.listen([&](int d1, int d2, int d3) {
        data1 = d1;
        data2 = d2;
        data3 = d3;
    });

    signal.emit(1, 2, 3);
    EXPECT(data1, 1);
    EXPECT(data2, 2);
    EXPECT(data3, 3);
}

void ref(int& ret) {
    int            count = 0;
    int            data  = 0;

    CSignalT<int&> signal;
    auto           l1 = signal.listen([&](int& v) { v += 1; });
    auto           l2 = signal.listen([&](int v) { count += v; });
    signal.emit(data);

    CSignalT<const int&> constSignal;
    auto                 l3 = constSignal.listen([&](const int& v) { count += v; });
    auto                 l4 = constSignal.listen([&](int v) { count += v; });
    constSignal.emit(data);

    EXPECT(data, 1);
    EXPECT(count, 3);
}

void refMany(int& ret) {
    int                        count = 0;
    int                        data1 = 0;
    int                        data2 = 10;

    CSignalT<int&, const int&> signal;
    auto                       l1 = signal.listen([&](int& v, const int&) { v += 1; });
    auto                       l2 = signal.listen([&](int v1, int v2) { count += v1 + v2; });

    signal.emit(data1, data2);
    EXPECT(data1, 1);
    EXPECT(count, 11);
}

void autoRefTypes(int& ret) {
    class CCopyCounter {
      public:
        CCopyCounter(int& createCount, int& destroyCount) : createCount(createCount), destroyCount(destroyCount) {
            createCount += 1;
        }

        CCopyCounter(CCopyCounter&& other) noexcept : CCopyCounter(other.createCount, other.destroyCount) {}
        CCopyCounter(const CCopyCounter& other) noexcept : CCopyCounter(other.createCount, other.destroyCount) {}

        ~CCopyCounter() {
            destroyCount += 1;
        }

      private:
        int& createCount;
        int& destroyCount;
    };

    auto                   createCount  = 0;
    auto                   destroyCount = 0;

    CSignalT<CCopyCounter> signal;
    auto                   listener = signal.listen([](const CCopyCounter& counter) {});

    signal.emit(CCopyCounter(createCount, destroyCount));
    EXPECT(createCount, 1);
    EXPECT(destroyCount, 1);
}

void forward(int& ret) {
    int           count = 0;

    CSignalT<int> sig;
    CSignalT<int> connected1;
    CSignalT<>    connected2;

    auto          conn1 = sig.forward(connected1);
    auto          conn2 = sig.forward(connected2);

    auto          listener1 = connected1.listen([&](int v) { count += v; });
    auto          listener2 = connected2.listen([&] { count += 1; });

    sig.emit(2);

    EXPECT(count, 3);
}

void listenerAdded(int& ret) {
    int                 count = 0;

    CSignalT<>          signal;
    CHyprSignalListener secondListener;

    auto                listener = signal.listen([&] {
        count += 1;

        if (!secondListener)
            secondListener = signal.listen([&] { count += 1; });
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

    auto                firstListener = signal.listen([&] {
        removedListener.reset(); // dropped and should NOT be invoked

        if (!addedListener)
            addedListener = signal.listen([&] { count += 2; });
    });

    removedListener = signal.listen([&] { count += 1; });

    signal.emit();
    EXPECT(count, 0); // neither the removed nor added listeners should fire

    signal.emit();
    EXPECT(count, 2); // only the new listener should fire
}

void signalDestroyed(int& ret) {
    int  count = 0;

    auto signal = std::make_unique<CSignalT<>>();

    // This ensures a destructor of a listener called before signal reset is safe.
    auto preListener = signal->listen([&] { count += 1; });

    auto listener = signal->listen([&] { signal.reset(); });

    // This ensures a destructor of a listener called after signal reset is safe
    // and gets called.
    auto postListener = signal->listen([&] { count += 1; });

    signal->emit();
    EXPECT(count, 2); // all listeners should fire regardless of signal deletion
}

// purely an asan test
void signalDestroyedBeforeListener() {
    CHyprSignalListener listener1;
    CHyprSignalListener listener2;

    CSignalT<>          signal;

    listener1 = signal.listen([] {});
    listener2 = signal.listen([] {});
}

void signalDestroyedWithAddedListener(int& ret) {
    int                 count = 0;

    auto                signal = std::make_unique<CSignalT<>>();
    CHyprSignalListener shouldNotRun;

    auto                listener = signal->listen([&] {
        shouldNotRun = signal->listen([&] { count += 2; });
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

    auto                listener = signal->listen([&] {
        removed.reset();
        shouldNotRun = signal->listen([&] { count += 2; });
        signal.reset();
    });

    removed = signal->listen([&] { count += 1; });

    signal->emit();
    EXPECT(count, 0);
}

void staticListener(int& ret) {
    int           data = 0;

    CSignalT<int> signal;
    signal.listenStatic([&](int newData) { data = newData; });

    signal.emit(1);
    EXPECT(data, 1);
}

void staticListenerDestroy(int& ret) {
    int  count = 0;

    auto signal = makeShared<CSignalT<>>();
    signal->listenStatic([&] { count += 1; });

    signal->listenStatic([&] {
        // should not fire but SHOULD be freed
        signal->listenStatic([&] { count += 3; });

        signal.reset();
    });

    signal->listenStatic([&] { count += 1; });

    signal->emit();
    EXPECT(count, 2);
}

// purely an asan test
void listenerDestroysSelf() {
    CSignalT<>          signal;

    CHyprSignalListener listener;
    listener = signal.listen([&] { listener.reset(); });

    // the static signal case is taken care of above

    signal.emit();
}

int main(int argc, char** argv, char** envp) {
    int ret = 0;
    legacy(ret);
    legacyListenerEmit(ret);
    legacyListeners(ret);
    empty(ret);
    typed(ret);
    ignoreParams(ret);
    typedMany(ret);
    ref(ret);
    refMany(ret);
    autoRefTypes(ret);
    forward(ret);
    listenerAdded(ret);
    lastListenerSwapped(ret);
    signalDestroyed(ret);
    signalDestroyedBeforeListener();
    signalDestroyedWithAddedListener(ret);
    signalDestroyedWithRemovedAndAddedListener(ret);
    staticListener(ret);
    staticListenerDestroy(ret);
    signalDestroyed(ret);
    listenerDestroysSelf();
    return ret;
}
