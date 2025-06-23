#include <hyprutils/signal/Listener.hpp>
#include <stdexcept>
#include <tuple>

using namespace Hyprutils::Signal;

Hyprutils::Signal::CSignalListener::CSignalListener(std::function<void(void*)> handler) : m_fHandler(handler) {
    ;
}

Hyprutils::Signal::CSignalListenerListEntry::~CSignalListenerListEntry() {
    if (m_previous) {
        m_previous->m_next = m_next;

        if (m_next) {
            m_next->m_previous = m_previous;
        } else {
            auto* head = m_previous;
            while (head->m_previous)
                head = head->m_previous;

            // The head of the list will always be a CSignalListenerListHead. On destruction, the head
            // unlinks m_previous of all elements ensuring this is the case.
            static_cast<CSignalListenerListHead*>(head)->removeLast(); // NOLINT: always a ListHead
        }
    } else {
        // If m_previous is null, the list head has been destroyed, and any m_next pointers are unreliable.
        // As the head as been destroyed, they will never be used again and do not need to be unlinked.
    }
}

Hyprutils::Signal::CSignalListenerListHead::CSignalListenerListHead(bool ownsListeners) : m_ownsListeners(ownsListeners) {}

Hyprutils::Signal::CSignalListenerListHead::~CSignalListenerListHead() {
    auto  seenCurrent  = false;
    auto  seenLastEmit = false;

    auto* list = m_next;
    while (list) {
        auto entry = list;
        list       = list->m_next;

        // If the head owns its listeners, destroy all listeners that arent currently in the exec chain.
        if (m_ownsListeners && !seenCurrent) {
            if (entry != m_currentListener) {
                // delete any already-called emitters (or all of them if not currently emitting)
                delete entry;
                continue;
            }

            seenCurrent = true;
        }

        // Unlink m_previous for all nodes. This ensures removeLast() will never be called on a non head node.
        // and acts as a flag to avoid accessing the head.
        entry->m_previous = nullptr;

        // If the listener is past the last emit marker, it won't execute or destroy itself.
        if (m_ownsListeners && seenLastEmit)
            delete entry;

        // Nulling m_next prevents listeners added in the same emit as signal destruction
        // after m_last has been changed from being fired.
        if (m_lastEmit && entry == m_lastEmit) {
            entry->m_next = nullptr;
            seenLastEmit  = true;
        }
    }
}

void Hyprutils::Signal::CSignalListenerListHead::append(CSignalListener* listener) {
    auto tail            = m_last ? static_cast<CSignalListenerListEntry*>(m_last) : this;
    tail->m_next         = listener;
    listener->m_previous = tail;
    listener->m_next     = nullptr;
    m_last               = listener;
}

void Hyprutils::Signal::CSignalListenerListHead::removeLast() {
    auto prev = m_last->m_previous;
    // NOLINTNEXTLINE: new tail cannot be the head meaning it must be a listener
    auto newTail = prev == this ? nullptr : static_cast<CSignalListener*>(prev);

    // If we're removing a handler that was scheduled to be run as part of the
    // current emit, we move the pointer back to exclude it.
    if (m_lastEmit == m_last)
        m_lastEmit = newTail;

    m_last = newTail;
}

bool Hyprutils::Signal::CSignalListenerListHead::emitAll(void* args) {
    if (!m_next)
        return true;

    if (m_lastEmit)
        throw std::logic_error("CSignal is not reentrant.");

    // Stopping at m_last prevents any handlers added as part of an emit callback from being triggered.
    m_lastEmit = m_last;
    auto live  = m_next->emitUntil(args, this, m_ownsListeners);

    // The signal and its list head can be destroyed during emit. emitUntil returns true
    // if this has not happened.
    if (live) {
        m_lastEmit        = nullptr;
        m_currentListener = nullptr;
    }

    return live;
}

bool Hyprutils::Signal::CSignalListener::emitUntil(void* data, CSignalListenerListHead* head, bool headOwnsListeners) {
    // If the head is owning, it needs to know which listener is currently running.
    // It uses this to destroy all prior listeners if the signal itself is destroyed.
    // Current and future listeners are expected to destroy themselves as seen below.
    if (headOwnsListeners && m_previous)
        head->m_currentListener = this;

    emitInternal(data);

    auto next     = m_next;
    auto previous = m_previous;

    // As a result of emitInternal, the signal might be destroyed. m_previous being null
    // marks this case. m_previous will be the head or another node in all other cases.
    // If the head owns listeners and is destroyed during emit, listeners should be destroyed
    // after execution.
    if (!previous && headOwnsListeners)
        delete this;

    // following the above delete, no fields of this can be accessed.

    if (next) {
        // The lastEmit pointer marks the last listener that should be executed.
        // If the signal is destroted, m_next will be unlinked at the last node already,
        // rendering the lastEmit pointer redundant.
        if (!previous || this != head->m_lastEmit) [[gnu::musttail]]
            return next->emitUntil(data, head, headOwnsListeners);
    }

    // If we're at the end of the list, return true if the signal has not been destroyed.
    // See above comment for more details.
    return previous;
}

void Hyprutils::Signal::CSignalListener::emitInternal(void* data) {
    if (!m_fHandler)
        return;

    m_fHandler(data);
}

void Hyprutils::Signal::CSignalListener::emit(std::any data) {
    auto dataTuple = std::tuple<std::any>(data);
    emitInternal(static_cast<void*>(&dataTuple));
}
