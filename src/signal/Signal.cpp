#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <algorithm>

using namespace Hyprutils::Signal;
using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer

void Hyprutils::Signal::CUntypedSignal::emitInternal(void* args) {
    auto live = m_listeners.emitAll(args);

    if (live)
        m_staticListeners.emitAll(args);
}

CHyprSignalListener Hyprutils::Signal::CUntypedSignal::registerListenerInternal(std::function<void(void*)> handler) {
    CHyprSignalListener listener = SP<CSignalListener>(new CSignalListener(handler));
    m_listeners.append(listener.get());
    return listener;
}

void Hyprutils::Signal::CUntypedSignal::registerStaticListenerInternal(std::function<void(void*)> handler) {
    m_staticListeners.append(new CSignalListener(handler));
}

void Hyprutils::Signal::CSignal::emit(std::any data) {
    CSignalT::emit(data);
}

CHyprSignalListener Hyprutils::Signal::CSignal::registerListener(std::function<void(std::any)> handler) {
    return CSignalT::registerListener(handler);
}

void Hyprutils::Signal::CSignal::registerStaticListener(std::function<void(void*, std::any)> handler, void* owner) {
    CSignalT<std::any>::registerStaticListener<void>(handler, owner);
}
