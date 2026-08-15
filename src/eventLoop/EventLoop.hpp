#pragma once

#include <hyprutils/eventLoop/EventLoop.hpp>
#include <hyprutils/memory/WeakPtr.hpp>

#include <deque>
#include <unordered_map>
#include <vector>

#include "Executor.hpp"
#include "backend/Backend.hpp"

namespace Hyprutils::EventLoop {
    class CFDSource;
    class CPostDispatchHook;
    class CTimer;

    class CEventLoop : public IEventLoop {
      public:
        CEventLoop() = default;
        virtual ~CEventLoop();

        void                                                                  setSelf(const Memory::CSharedPointer<CEventLoop>& self);
        std::expected<void, std::string>                                      init();

        virtual std::expected<Memory::CSharedPointer<IFDSource>, std::string> addFD(OS::CFileDescriptor&& fd, FdEventMask mask,
                                                                                    std::function<void(IFDSource&, FdEventMask)>&& callback) override;
        virtual Memory::CSharedPointer<ITimer>              addTimer(std::optional<std::chrono::steady_clock::duration> timeout, std::function<void(ITimer&)>&& callback) override;
        virtual void                                        addIdle(std::function<void()> fn) override;
        virtual Memory::CSharedPointer<IPostDispatchHook>   addPostDispatch(std::function<void()>&& callback) override;
        virtual Memory::CAtomicSharedPointer<ILoopExecutor> executor() override;
        virtual int                                         fd() const override;
        virtual std::expected<void, std::string>            dispatch() override;
        virtual std::expected<void, std::string>            enterLoop() override;
        virtual void                                        stop() override;

        std::expected<void, std::string>                    updateSourceMask(CFDSource& source, FdEventMask mask);
        void                                                removeSource(CFDSource& source);
        void                                                timerChanged(bool cleanup = true);

      private:
        std::expected<Memory::CSharedPointer<CFDSource>, std::string>    addFDInternal(OS::CFileDescriptor&& fd, FdEventMask mask,
                                                                                       std::function<void(IFDSource&, FdEventMask)>&& callback);
        std::expected<void, std::string>                                 dispatchCycle(int timeout);
        std::expected<void, std::string>                                 drainReady(int timeout);
        void                                                             dispatchTimers();
        void                                                             dispatchIdles();
        void                                                             dispatchPostHooks();
        void                                                             drainExecutor();
        void                                                             wakeIdle();
        void                                                             detachAll();

        Memory::CUniquePointer<CEventLoopBackend>                        m_backend;
        uintptr_t                                                        m_nextID      = 1;
        bool                                                             m_dispatching = false;
        bool                                                             m_running     = false;
        std::optional<std::string>                                       m_pendingError;

        Memory::CWeakPointer<CEventLoop>                                 m_self;

        std::unordered_map<uintptr_t, Memory::CSharedPointer<CFDSource>> m_sources;
        std::vector<Memory::CWeakPointer<CTimer>>                        m_timers;
        std::vector<Memory::CWeakPointer<CPostDispatchHook>>             m_postHooks;
        std::deque<std::function<void()>>                                m_idles;

        Memory::CAtomicSharedPointer<SExecutorState>                     m_executorState;
        Memory::CAtomicSharedPointer<ILoopExecutor>                      m_executor;
    };
}
