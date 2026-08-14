#pragma once

#include <hyprutils/eventLoop/Executor.hpp>
#include <hyprutils/memory/Atomic.hpp>
#include <hyprutils/os/FileDescriptor.hpp>

#include <deque>
#include <mutex>

namespace Hyprutils::EventLoop {
    struct SExecutorState {
        std::mutex                        mutex;
        std::deque<std::function<void()>> callbacks;
        OS::CFileDescriptor               eventFD;
        bool                              active = true;
    };

    class CLoopExecutor : public ILoopExecutor {
      public:
        CLoopExecutor(Memory::CAtomicSharedPointer<SExecutorState> state);
        virtual ~CLoopExecutor() = default;

        virtual void post(std::function<void()> fn) override;

      private:
        Memory::CAtomicSharedPointer<SExecutorState> m_state;
    };
}
