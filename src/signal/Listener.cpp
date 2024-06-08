#include <hyprutils/signal/Listener.hpp>

using namespace Hyprutils::Signal;

Hyprutils::Signal::CSignalListener::CSignalListener(std::function<void(std::any)> handler) : m_fHandler(handler) {
    ;
}

void Hyprutils::Signal::CSignalListener::emit(std::any data) {
    if (!m_fHandler)
        return;

    m_fHandler(data);
}

Hyprutils::Signal::CStaticSignalListener::CStaticSignalListener(std::function<void(void*, std::any)> handler, void* owner) : m_pOwner(owner), m_fHandler(handler) {
    ;
}

void Hyprutils::Signal::CStaticSignalListener::emit(std::any data) {
    m_fHandler(m_pOwner, data);
}
