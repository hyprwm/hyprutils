#pragma once

#include <expected>
#include <functional>
#include <optional>
#include <string>

#include "EventMask.hpp"
#include "Executor.hpp"
#include "FDSource.hpp"
#include "PostDispatchHook.hpp"
#include "Timer.hpp"

#include "../memory/Atomic.hpp"
#include "../memory/SharedPtr.hpp"
#include "../os/FileDescriptor.hpp"

namespace Hyprutils::EventLoop {
    /*
     * A strictly single-threaded event loop. Nothing in this event loop is thread-safe,
     * and everything is dispatched synchronously.
     * For executing on the loop from another thread, use an Executor.
     */
    class IEventLoop {
      public:
        virtual ~IEventLoop() = default;

        IEventLoop(const IEventLoop&)                                                                      = delete;
        IEventLoop(IEventLoop&&)                                                                           = delete;
        IEventLoop&                                                           operator=(const IEventLoop&) = delete;
        IEventLoop&                                                           operator=(IEventLoop&&)      = delete;

        static std::expected<Memory::CSharedPointer<IEventLoop>, std::string> create();

        // The event loop takes ownership of fd. HUP and ERROR are reported while the
        // source is enabled and must not be included in mask. An empty mask disables the
        // source completely. Readiness callbacks must make progress because
        // dispatch() drains level-triggered events until quiet (with a fairness limit).
        virtual std::expected<Memory::CSharedPointer<IFDSource>, std::string> addFD(OS::CFileDescriptor&& fd, FdEventMask mask,
                                                                                    std::function<void(IFDSource&, FdEventMask)>&& callback) = 0;

        // Timers are weakly owned by the loop. Dropping the returned pointer cancels
        // the timer. A fired timer is disarmed and can be rearmed from its callback.
        virtual Memory::CSharedPointer<ITimer> addTimer(std::optional<std::chrono::steady_clock::duration> timeout, std::function<void(ITimer&)>&& callback) = 0;

        // Idles added by another idle are deferred to the next dispatch cycle.
        virtual void addIdle(std::function<void()> fn) = 0;

        // Hooks are weakly owned by the loop and run after the idle callback snapshot.
        virtual Memory::CSharedPointer<IPostDispatchHook> addPostDispatch(std::function<void()>&& callback) = 0;

        // This is the only event-loop object intended for cross-thread use.
        virtual Memory::CAtomicSharedPointer<ILoopExecutor> executor() = 0;

        // The returned poll-readable descriptor remains owned by the event loop.
        virtual int                              fd() const  = 0;
        virtual std::expected<void, std::string> dispatch()  = 0;
        virtual std::expected<void, std::string> enterLoop() = 0;
        // To stop the loop from another thread, post a stop() call through executor().
        virtual void stop() = 0;

      private:
        IEventLoop() = default;
        friend class CEventLoop;
    };
}
