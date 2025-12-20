#include "hyprutils/memory/SharedPtr.hpp"
#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <algorithm>

using namespace Hyprutils::Signal;
using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer

void Hyprutils::Signal::CSignalBase::emitInternal(void* args) {
    if (!m_vListeners.empty()) {
        std::vector<SP<CSignalListener>> listeners;
        listeners.reserve(m_vListeners.size());

        for (const auto& l : m_vListeners) {
            if (l.expired())
                continue;

            listeners.emplace_back(l.lock());
        }

        for (const auto& l : listeners) {
            // if there is only one lock, it means the event is only held by the listeners
            // vector and was removed during our iteration
            if (l.strongRef() == 1)
                continue;

            l->emitInternal(args);
        }

        // release SPs
        listeners.clear();
    }

    if (!m_vStaticListeners.empty()) {
        const auto statics = m_vStaticListeners;

        for (const auto& l : statics) {
            l->emitInternal(args);
        }
    }

    // we cannot release any expired refs here as one of the listeners could've removed this object and
    // as such we'd be doing a UAF
}

CHyprSignalListener Hyprutils::Signal::CSignalBase::registerListenerInternal(std::function<void(void*)> handler) {
    CHyprSignalListener listener = SP<CSignalListener>(new CSignalListener(handler));
    m_vListeners.emplace_back(listener);

    // housekeeping: remove any stale listeners
    std::erase_if(m_vListeners, [](const auto& other) { return other.expired(); });

    return listener;
}

void Hyprutils::Signal::CSignalBase::registerStaticListenerInternal(std::function<void(void*)> handler) {
    m_vStaticListeners.emplace_back(SP<CSignalListener>(new CSignalListener(handler)));
}

void Hyprutils::Signal::CSignal::emit(std::any data) {
    CSignalT::emit(data);
}
