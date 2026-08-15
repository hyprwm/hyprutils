#include "Executor.hpp"

using namespace Hyprutils::EventLoop;

CLoopExecutor::CLoopExecutor(Memory::CAtomicSharedPointer<SExecutorState> state) : m_state(std::move(state)) {
    ;
}

void CLoopExecutor::post(std::function<void()> fn) {
    if (!fn)
        return;

    std::lock_guard lock(m_state->mutex);
    if (!m_state->active || !m_state->wake)
        return;

    m_state->callbacks.emplace_back(std::move(fn));
    m_state->wake();
}
