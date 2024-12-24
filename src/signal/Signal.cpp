#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <algorithm>

using namespace Hyprutils::Signal;
using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer

void Hyprutils::Signal::CSignal::emit(std::any data) {
    std::vector<SP<CSignalListener>> listeners;
    for (auto& l : m_vListeners) {
        if (l.expired())
            continue;

        listeners.emplace_back(l.lock());
    }

    std::vector<CStaticSignalListener*> statics;
    for (auto& l : m_vStaticListeners) {
        statics.emplace_back(l.get());
    }

    for (auto& l : listeners) {
        // if there is only one lock, it means the event is only held by the listeners
        // vector and was removed during our iteration
        if (l.strongRef() == 1)
            continue;

        l->emit(data);
    }

    for (auto& l : statics) {
        l->emit(data);
    }

    // release SPs
    listeners.clear();

    // we cannot release any expired refs here as one of the listeners could've removed this object and
    // as such we'd be doing a UAF
}

CHyprSignalListener Hyprutils::Signal::CSignal::registerListener(std::function<void(std::any)> handler) {
    CHyprSignalListener listener = makeShared<CSignalListener>(handler);
    m_vListeners.emplace_back(listener);

    // housekeeping: remove any stale listeners
    std::erase_if(m_vListeners, [](const auto& other) { return other.expired(); });

    return listener;
}

void Hyprutils::Signal::CSignal::registerStaticListener(std::function<void(void*, std::any)> handler, void* owner) {
    m_vStaticListeners.emplace_back(std::make_unique<CStaticSignalListener>(handler, owner));
}