#pragma once

#include "hyprutils/eventLoop/EventLoop.hpp"
#include <hyprutils/eventLoop/Timer.hpp>

#include <chrono>

namespace Hyprutils::EventLoop {
    using Timestamp = std::chrono::steady_clock::time_point;
    using Duration  = std::chrono::steady_clock::duration;

    class CTimer : public ITimer {
      public:
        CTimer(CEventLoop& loop, std::optional<Duration> dur, std::function<void(ITimer&)>&& callback);

        virtual ~CTimer();

        virtual void             updateTimeout(std::optional<Duration> timeout) override;

        std::optional<Timestamp> expiresAt() const;
        bool                     expired() const;
        void                     fire();
        void                     detach();

      private:
        CEventLoop*                  m_loop = nullptr;
        std::optional<Timestamp>     m_expiry;
        std::function<void(ITimer&)> m_callback;
    };
}
