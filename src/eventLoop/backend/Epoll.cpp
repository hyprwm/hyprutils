#include "Backend.hpp"

#include <hyprutils/os/FileDescriptor.hpp>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <unordered_map>

using namespace Hyprutils::EventLoop;
using namespace Hyprutils::Memory;
using namespace Hyprutils::OS;

namespace {
    constexpr uintptr_t TIMER_ID = std::numeric_limits<uintptr_t>::max();
    constexpr uintptr_t WAKE_ID  = 0;

    std::string         systemError(const std::string& operation) {
        return operation + ": " + std::strerror(errno);
    }

    uint32_t epollMask(FdEventMask mask) {
        uint32_t events = 0;
        if (mask & eEventMask::READABLE)
            events |= EPOLLIN;
        if (mask & eEventMask::WRITABLE)
            events |= EPOLLOUT;
        events |= EPOLLRDHUP;
        return events;
    }

    FdEventMask eventMask(uint32_t events) {
        FdEventMask mask = eEventMask::EMPTY;
        if (events & EPOLLIN)
            mask |= eEventMask::READABLE;
        if (events & EPOLLOUT)
            mask |= eEventMask::WRITABLE;
        if (events & (EPOLLHUP | EPOLLRDHUP))
            mask |= eEventMask::HUP;
        if (events & EPOLLERR)
            mask |= eEventMask::ERROR;
        return mask;
    }

    void drainCounterFD(int fd) {
        uint64_t value = 0;
        while (read(fd, &value, sizeof(value)) < 0 && errno == EINTR) {}
    }
}

struct CEventLoopBackend::SBackendState {
    struct SRegistration {
        int         fd         = -1;
        FdEventMask mask       = eEventMask::EMPTY;
        bool        registered = false;
    };

    CFileDescriptor                              epollFD;
    CFileDescriptor                              timerFD;
    CFileDescriptor                              wakeFD;
    std::unordered_map<uintptr_t, SRegistration> registrations;
};

std::expected<CUniquePointer<CEventLoopBackend>, std::string> CEventLoopBackend::create() {
    CUniquePointer<CEventLoopBackend> backend{new CEventLoopBackend};
    backend->m_state = makeUnique<SBackendState>();

    backend->m_state->epollFD = CFileDescriptor{epoll_create1(EPOLL_CLOEXEC)};
    if (!backend->m_state->epollFD.isValid())
        return std::unexpected(systemError("epoll_create1"));

    backend->m_state->timerFD = CFileDescriptor{timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK)};
    if (!backend->m_state->timerFD.isValid())
        return std::unexpected(systemError("timerfd_create"));

    backend->m_state->wakeFD = CFileDescriptor{eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)};
    if (!backend->m_state->wakeFD.isValid())
        return std::unexpected(systemError("eventfd"));

    epoll_event timerEvent = {
        .events = EPOLLIN,
        .data   = {.u64 = TIMER_ID},
    };
    if (epoll_ctl(backend->m_state->epollFD.get(), EPOLL_CTL_ADD, backend->m_state->timerFD.get(), &timerEvent) < 0)
        return std::unexpected(systemError("epoll_ctl(ADD timer)"));

    epoll_event wakeEvent = {
        .events = EPOLLIN,
        .data   = {.u64 = WAKE_ID},
    };
    if (epoll_ctl(backend->m_state->epollFD.get(), EPOLL_CTL_ADD, backend->m_state->wakeFD.get(), &wakeEvent) < 0)
        return std::unexpected(systemError("epoll_ctl(ADD wake)"));

    return backend;
}

CEventLoopBackend::~CEventLoopBackend() = default;

int CEventLoopBackend::fd() const {
    return m_state->epollFD.get();
}

std::expected<void, std::string> CEventLoopBackend::addFD(uintptr_t sourceID, int fd, FdEventMask mask) {
    if (m_state->registrations.contains(sourceID))
        return std::unexpected("file descriptor source is already registered");

    SBackendState::SRegistration registration = {
        .fd         = fd,
        .mask       = mask,
        .registered = false,
    };

    if (mask) {
        epoll_event event = {
            .events = epollMask(mask),
            .data   = {.u64 = sourceID},
        };
        if (epoll_ctl(m_state->epollFD.get(), EPOLL_CTL_ADD, fd, &event) < 0)
            return std::unexpected(systemError("epoll_ctl(ADD)"));
        registration.registered = true;
    }

    m_state->registrations.emplace(sourceID, registration);
    return {};
}

std::expected<void, std::string> CEventLoopBackend::updateFD(uintptr_t sourceID, int fd, FdEventMask mask) {
    const auto registrationIt = m_state->registrations.find(sourceID);
    if (registrationIt == m_state->registrations.end() || registrationIt->second.fd != fd)
        return std::unexpected("file descriptor source has been removed");

    auto& registration = registrationIt->second;
    if (!mask && registration.registered) {
        if (epoll_ctl(m_state->epollFD.get(), EPOLL_CTL_DEL, fd, nullptr) < 0)
            return std::unexpected(systemError("epoll_ctl(DEL)"));
        registration.registered = false;
    } else if (mask) {
        epoll_event event = {
            .events = epollMask(mask),
            .data   = {.u64 = sourceID},
        };
        const auto operation = registration.registered ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        if (epoll_ctl(m_state->epollFD.get(), operation, fd, &event) < 0)
            return std::unexpected(systemError(operation == EPOLL_CTL_MOD ? "epoll_ctl(MOD)" : "epoll_ctl(ADD)"));
        registration.registered = true;
    }

    registration.mask = mask;
    return {};
}

void CEventLoopBackend::removeFD(uintptr_t sourceID, int fd) {
    const auto registrationIt = m_state->registrations.find(sourceID);
    if (registrationIt == m_state->registrations.end() || registrationIt->second.fd != fd)
        return;

    if (registrationIt->second.registered)
        epoll_ctl(m_state->epollFD.get(), EPOLL_CTL_DEL, fd, nullptr);
    m_state->registrations.erase(registrationIt);
}

std::expected<void, std::string> CEventLoopBackend::rearmFD(uintptr_t sourceID, int fd, FdEventMask mask) {
    return {};
}

std::expected<void, std::string> CEventLoopBackend::armTimer(std::optional<std::chrono::steady_clock::time_point> deadline) {
    itimerspec spec = {};
    if (deadline) {
        auto remaining = *deadline - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::steady_clock::duration::zero())
            remaining = std::chrono::nanoseconds{1};

        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(remaining);
        if (seconds.count() > std::numeric_limits<time_t>::max())
            seconds = std::chrono::seconds{std::numeric_limits<time_t>::max()};
        const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(remaining - seconds);
        spec.it_value    = {
            .tv_sec  = seconds.count(),
            .tv_nsec = std::min<int64_t>(nanos.count(), 999999999),
        };
    }

    if (timerfd_settime(m_state->timerFD.get(), 0, &spec, nullptr) < 0)
        return std::unexpected(systemError("timerfd_settime"));
    return {};
}

void CEventLoopBackend::signalWake() {
    const uint64_t value = 1;
    while (write(m_state->wakeFD.get(), &value, sizeof(value)) < 0) {
        if (errno == EINTR)
            continue;
        return;
    }
}

std::expected<std::vector<SBackendEvent>, std::string> CEventLoopBackend::wait(int timeout, size_t maximumEvents) {
    std::vector<epoll_event> events(std::max<size_t>(1, maximumEvents));

    int                      count = 0;
    while ((count = epoll_wait(m_state->epollFD.get(), events.data(), events.size(), timeout)) < 0) {
        if (errno == EINTR)
            continue;
        return std::unexpected(systemError("epoll_wait"));
    }

    std::vector<SBackendEvent> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        if (events[i].data.u64 == TIMER_ID) {
            drainCounterFD(m_state->timerFD.get());
            result.push_back({.type = eBackendEventType::TIMER});
        } else if (events[i].data.u64 == WAKE_ID) {
            drainCounterFD(m_state->wakeFD.get());
            result.push_back({.type = eBackendEventType::WAKE});
        } else {
            const auto sourceID = events[i].data.u64;
            result.push_back({.type = eBackendEventType::FD, .sourceID = sourceID, .events = eventMask(events[i].events)});
        }
    }

    return result;
}
