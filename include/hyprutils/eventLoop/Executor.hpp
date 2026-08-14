#pragma once

#include <functional>

namespace Hyprutils::EventLoop {
    class CEventLoop;

    /*
     * A thread-safe executor. This will post the fn to be executed on the main loop
     * whenever the loop is free to do so.
     */
    class ILoopExecutor {
      public:
        virtual ~ILoopExecutor() = default;

        ILoopExecutor(const ILoopExecutor&)            = delete;
        ILoopExecutor(ILoopExecutor&&)                 = delete;
        ILoopExecutor& operator=(const ILoopExecutor&) = delete;
        ILoopExecutor& operator=(ILoopExecutor&&)      = delete;

        /*
         * Post to the main event loop. This function will be added to the event loop's
         * idle callbacks, and will be called synchronously once the event loop is drained,
         * or on the next dispatch if it's empty. Posts made after loop destruction are discarded.
         */
        virtual void post(std::function<void()> fn) = 0;

      private:
        ILoopExecutor() = default;
        friend class CLoopExecutor;
    };
}
