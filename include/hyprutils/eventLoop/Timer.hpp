#pragma once

#include <chrono>
#include <optional>

namespace Hyprutils::EventLoop {

    /*
     * A basic timer for the event loop. This timer is a one-shot timer by design,
     * but can be a repeat timer by simply re-arming it in the callback.
     * Re-arming with an already elapsed timeout may fire it again in the same dispatch.
     */
    class ITimer {
      public:
        virtual ~ITimer() = default;

        ITimer(const ITimer&)            = delete;
        ITimer(ITimer&&)                 = delete;
        ITimer& operator=(const ITimer&) = delete;
        ITimer& operator=(ITimer&&)      = delete;

        /*
         * Update this timer's timeout. Passing std::nullopt will disarm the timer.
         * If the timer is already pending, this will not call it - e.g. if the timer is to
         * fire in 2137ms, but you call updateTimeout(500ms), it will fire in 500ms, or with
         * updateTimeout(3000ms), in 3s.
         */
        virtual void updateTimeout(std::optional<std::chrono::steady_clock::duration> timeout) = 0;

      private:
        ITimer() = default;

        friend class CTimer;
    };
}
