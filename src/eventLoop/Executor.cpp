#include "Executor.hpp"

#include <cerrno>
#include <cstdint>
#include <unistd.h>

using namespace Hyprutils::EventLoop;

CLoopExecutor::CLoopExecutor(Memory::CAtomicSharedPointer<SExecutorState> state) : m_state(std::move(state)) {
    ;
}

void CLoopExecutor::post(std::function<void()> fn) {
    if (!fn)
        return;

    std::lock_guard lock(m_state->mutex);
    if (!m_state->active || !m_state->eventFD.isValid())
        return;

    m_state->callbacks.emplace_back(std::move(fn));

    const uint64_t value = 1;
    while (write(m_state->eventFD.get(), &value, sizeof(value)) < 0) {
        if (errno == EINTR)
            continue;
        return;
    }
}
