#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include "shared.hpp"

using namespace Hyprutils::Signal;
using namespace Hyprutils::Memory;

int main(int argc, char** argv, char** envp) {
    int     ret = 0;

    CSignal signal;
    int     data     = 0;
    auto    listener = signal.registerListener([&]([[maybe_unused]] std::any d) { data = 1; });

    signal.emit();

    EXPECT(data, 1);

    data = 0;

    listener.reset();

    signal.emit();

    EXPECT(data, 0);

    return ret;
}