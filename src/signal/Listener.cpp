#include <hyprutils/signal/Listener.hpp>
#include <tuple>

using namespace Hyprutils::Signal;

Hyprutils::Signal::CSignalListener::CSignalListener(std::function<void(void*)> handler) : m_fHandler(handler) {
    ;
}

void Hyprutils::Signal::CSignalListener::emitInternal(void* data) {
    if (!m_fHandler)
        return;

    m_fHandler(data);
}

void Hyprutils::Signal::CSignalListener::emit(std::any data) {
    auto dataTuple = std::tuple<std::any>(data);
    emitInternal(&dataTuple);
}
