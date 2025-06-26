#include <any>
#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
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

void staticListener(int& ret) {
    int           data = 0;

    CSignalT<int> signal;
    signal.listenStatic([&](int newData) { data = newData; });

    signal.emit(1);
    EXPECT(data, 1);
}

int main(int argc, char** argv, char** envp) {
    int ret = 0;
    legacy(ret);
    legacyListenerEmit(ret);
    empty(ret);
    typed(ret);
    typedMany(ret);
    staticListener(ret);
    return ret;
}
