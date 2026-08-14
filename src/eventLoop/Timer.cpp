#include "Timer.hpp"

#include "EventLoop.hpp"

using namespace Hyprutils::EventLoop;

CTimer::CTimer(CEventLoop& loop, std::optional<Duration> dur, std::function<void(ITimer&)>&& callback) : m_loop(&loop), m_callback(std::move(callback)) {
    updateTimeout(dur);
}

CTimer::~CTimer() {
    if (m_loop)
        m_loop->timerChanged(false);
}

void CTimer::updateTimeout(std::optional<Duration> timeout) {
    if (!timeout) {
        m_expiry.reset();
    } else {
        const auto now = std::chrono::steady_clock::now();
        if (*timeout <= Duration::zero())
            m_expiry = now;
        else if (*timeout >= Timestamp::max() - now)
            m_expiry = Timestamp::max();
        else
            m_expiry = now + *timeout;
    }

    if (m_loop)
        m_loop->timerChanged();
}

std::optional<Timestamp> CTimer::expiresAt() const {
    return m_expiry;
}

bool CTimer::expired() const {
    return m_expiry && std::chrono::steady_clock::now() >= *m_expiry;
}

void CTimer::fire() {
    m_expiry.reset();
    if (m_callback)
        m_callback(*this);
}

void CTimer::detach() {
    m_loop = nullptr;
    m_expiry.reset();
}
