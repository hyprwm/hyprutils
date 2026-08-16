#include "Backend.hpp"

#include <hyprutils/os/FileDescriptor.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <condition_variable>
#include <fcntl.h>
#include <limits>
#include <mutex>
#include <poll.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <system_error>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>
#include <utility>

using namespace Hyprutils::EventLoop;
using namespace Hyprutils::Memory;
using namespace Hyprutils::OS;

namespace {
    constexpr uintptr_t TIMER_IDENT = 1;

    using KqueueUserData = decltype(std::declval<struct kevent>().udata);

    template <typename T = KqueueUserData>
    T sourceToken(uintptr_t sourceID) {
        if constexpr (std::is_pointer_v<T>)
            return reinterpret_cast<T>(sourceID);
        else
            return static_cast<T>(sourceID);
    }

    template <typename T = KqueueUserData>
    uintptr_t sourceID(T token) {
        if constexpr (std::is_pointer_v<T>)
            return reinterpret_cast<uintptr_t>(token);
        else
            return static_cast<uintptr_t>(token);
    }

    std::string systemError(const std::string& operation, int error = errno) {
        return operation + ": " + std::strerror(error);
    }

    bool setDescriptorFlags(int fd) {
        if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
            return false;

        const int flags = fcntl(fd, F_GETFL);
        return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
    }

    void drainPipe(int fd) {
        std::array<char, 64> buffer;
        while (true) {
            const auto size = read(fd, buffer.data(), buffer.size());
            if (size > 0)
                continue;
            if (size < 0 && errno == EINTR)
                continue;
            return;
        }
    }

    void signalPipe(int fd) {
        const char byte = 1;
        while (write(fd, &byte, sizeof(byte)) < 0) {
            if (errno == EINTR)
                continue;
            return;
        }
    }

    FdEventMask filterMask(const struct kevent& event) {
        FdEventMask mask = eEventMask::EMPTY;
        if (event.filter == EVFILT_READ)
            mask |= eEventMask::READABLE;
        else if (event.filter == EVFILT_WRITE)
            mask |= eEventMask::WRITABLE;

        if (event.flags & EV_EOF) {
            mask |= eEventMask::HUP;
            if (event.fflags != 0)
                mask |= eEventMask::ERROR;
        }
        if (event.flags & EV_ERROR)
            mask |= eEventMask::ERROR;
        return mask;
    }
}

struct CEventLoopBackend::SBackendState {
    struct SRegistration {
        int         fd   = -1;
        FdEventMask mask = eEventMask::EMPTY;
    };

    CFileDescriptor                              kqueueFD;
    CFileDescriptor                              aggregateReadFD;
    CFileDescriptor                              aggregateWriteFD;
    CFileDescriptor                              controlReadFD;
    CFileDescriptor                              controlWriteFD;
    std::unordered_map<uintptr_t, SRegistration> registrations;
    bool                                         timerArmed = false;

    std::mutex                                   pendingMutex;
    std::unordered_map<uintptr_t, FdEventMask>   pendingFDs;
    bool                                         timerPending  = false;
    bool                                         wakePending   = false;
    bool                                         wakeRequested = false;
    std::optional<std::string>                   failure;
    uint64_t                                     syncRequested = 0;
    uint64_t                                     syncCompleted = 0;
    std::condition_variable                      syncCV;

    std::atomic<bool>                            stopping = false;
    std::thread                                  worker;
};

void CEventLoopBackend::notify(SBackendState& state) {
    signalPipe(state.aggregateWriteFD.get());
}

void CEventLoopBackend::fail(SBackendState& state, std::string error) {
    {
        std::lock_guard lock(state.pendingMutex);
        if (!state.failure)
            state.failure = std::move(error);
    }
    state.syncCV.notify_all();
    notify(state);
}

void CEventLoopBackend::runWorker(SBackendState* state) {
    std::array<struct kevent, 256> events;

    while (!state->stopping) {
        int count = kevent(state->kqueueFD.get(), nullptr, 0, events.data(), events.size(), nullptr);
        if (count < 0) {
            if (errno == EINTR)
                continue;
            fail(*state, systemError("kevent(wait)"));
            return;
        }

        std::unordered_map<uintptr_t, FdEventMask> readyFDs;
        bool                                       timerReady   = false;
        bool                                       controlReady = false;

        const auto                                 processEvents = [&](int eventCount) {
            for (int i = 0; i < eventCount; ++i) {
                const auto& event = events[i];
                if (event.filter == EVFILT_READ && event.ident == static_cast<uintptr_t>(state->controlReadFD.get()) && sourceID(event.udata) == 0) {
                    controlReady = true;
                    continue;
                }
                if (event.filter == EVFILT_TIMER && event.ident == TIMER_IDENT) {
                    timerReady = true;
                    continue;
                }

                const auto eventSourceID = sourceID(event.udata);
                if (eventSourceID == 0) {
                    if (event.flags & EV_ERROR)
                        fail(*state, systemError("kevent event", static_cast<int>(event.data)));
                    continue;
                }

                auto readyIt = readyFDs.try_emplace(eventSourceID, eEventMask::EMPTY).first;
                readyIt->second |= filterMask(event);
            }
        };

        processEvents(count);
        if (controlReady)
            drainPipe(state->controlReadFD.get());

        uint64_t syncTarget = 0;
        if (controlReady) {
            {
                std::lock_guard lock(state->pendingMutex);
                syncTarget = state->syncRequested;
            }

            const timespec zeroTimeout = {};
            while (true) {
                count = kevent(state->kqueueFD.get(), nullptr, 0, events.data(), events.size(), &zeroTimeout);
                if (count < 0 && errno == EINTR)
                    continue;
                if (count < 0) {
                    fail(*state, systemError("kevent(sync)"));
                    return;
                }
                if (count == 0)
                    break;

                controlReady = false;
                processEvents(count);
                if (controlReady) {
                    drainPipe(state->controlReadFD.get());
                    std::lock_guard lock(state->pendingMutex);
                    syncTarget = state->syncRequested;
                }
            }
        }

        if (state->stopping)
            return;

        bool wokeThisRound = false;
        {
            std::lock_guard lock(state->pendingMutex);
            for (const auto& [sourceID, mask] : readyFDs) {
                auto pendingIt = state->pendingFDs.try_emplace(sourceID, eEventMask::EMPTY).first;
                pendingIt->second |= mask;
            }
            state->timerPending  = state->timerPending || timerReady;
            wokeThisRound        = state->wakeRequested;
            state->wakePending   = state->wakePending || state->wakeRequested;
            state->wakeRequested = false;
            if (syncTarget > 0)
                state->syncCompleted = std::max(state->syncCompleted, syncTarget);
        }
        if (!readyFDs.empty() || timerReady || syncTarget > 0 || wokeThisRound)
            notify(*state);
        if (syncTarget > 0)
            state->syncCV.notify_all();
    }
}

std::expected<CUniquePointer<CEventLoopBackend>, std::string> CEventLoopBackend::create() {
    CUniquePointer<CEventLoopBackend> backend{new CEventLoopBackend};
    backend->m_state = makeUnique<SBackendState>();

    backend->m_state->kqueueFD = CFileDescriptor{kqueue()};
    if (!backend->m_state->kqueueFD.isValid())
        return std::unexpected(systemError("kqueue"));
    if (fcntl(backend->m_state->kqueueFD.get(), F_SETFD, FD_CLOEXEC) < 0)
        return std::unexpected(systemError("fcntl(FD_CLOEXEC)"));

    std::array<int, 2> pipeFDs = {-1, -1};
    if (pipe(pipeFDs.data()) < 0)
        return std::unexpected(systemError("pipe"));
    backend->m_state->aggregateReadFD  = CFileDescriptor{pipeFDs[0]};
    backend->m_state->aggregateWriteFD = CFileDescriptor{pipeFDs[1]};
    if (!setDescriptorFlags(pipeFDs[0]) || !setDescriptorFlags(pipeFDs[1]))
        return std::unexpected(systemError("fcntl(pipe)"));

    pipeFDs = {-1, -1};
    if (pipe(pipeFDs.data()) < 0)
        return std::unexpected(systemError("pipe"));
    backend->m_state->controlReadFD  = CFileDescriptor{pipeFDs[0]};
    backend->m_state->controlWriteFD = CFileDescriptor{pipeFDs[1]};
    if (!setDescriptorFlags(pipeFDs[0]) || !setDescriptorFlags(pipeFDs[1]))
        return std::unexpected(systemError("fcntl(pipe)"));

    struct kevent controlEvent;
    EV_SET(&controlEvent, backend->m_state->controlReadFD.get(), EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, KqueueUserData{});
    if (kevent(backend->m_state->kqueueFD.get(), &controlEvent, 1, nullptr, 0, nullptr) < 0)
        return std::unexpected(systemError("kevent(ADD control)"));

    try {
        backend->m_state->worker = std::thread{runWorker, backend->m_state.get()};
    } catch (const std::system_error& error) { return std::unexpected(std::string{"failed to create kqueue worker: "} + error.what()); }

    return backend;
}

CEventLoopBackend::~CEventLoopBackend() {
    if (!m_state)
        return;

    m_state->stopping = true;
    signalPipe(m_state->controlWriteFD.get());
    m_state->syncCV.notify_all();
    if (m_state->worker.joinable())
        m_state->worker.join();
}

int CEventLoopBackend::fd() const {
    return m_state->aggregateReadFD.get();
}

std::expected<void, std::string> CEventLoopBackend::addFD(uintptr_t sourceID, int fd, FdEventMask mask) {
    {
        std::lock_guard lock(m_state->pendingMutex);
        if (m_state->failure)
            return std::unexpected(*m_state->failure);
    }
    if (m_state->registrations.contains(sourceID))
        return std::unexpected("file descriptor source is already registered");

    std::array<struct kevent, 2> changes;
    int                          count = 0;
    if (mask & eEventMask::READABLE)
        EV_SET(&changes[count++], fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_DISPATCH, 0, 0, sourceToken(sourceID));
    if (mask & eEventMask::WRITABLE)
        EV_SET(&changes[count++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_DISPATCH, 0, 0, sourceToken(sourceID));

    if (count > 0 && kevent(m_state->kqueueFD.get(), changes.data(), count, nullptr, 0, nullptr) < 0) {
        auto error = systemError("kevent(ADD)");
        fail(*m_state, error);
        return std::unexpected(std::move(error));
    }

    m_state->registrations.emplace(sourceID, SBackendState::SRegistration{.fd = fd, .mask = mask});
    return {};
}

std::expected<void, std::string> CEventLoopBackend::updateFD(uintptr_t sourceID, int fd, FdEventMask mask) {
    {
        std::lock_guard lock(m_state->pendingMutex);
        if (m_state->failure)
            return std::unexpected(*m_state->failure);
    }

    const auto registrationIt = m_state->registrations.find(sourceID);
    if (registrationIt == m_state->registrations.end() || registrationIt->second.fd != fd)
        return std::unexpected("file descriptor source has been removed");

    const auto                   oldMask = registrationIt->second.mask;
    std::array<struct kevent, 2> changes;
    int                          count = 0;

    const auto                   updateFilter = [&](eEventMask eventMask, int32_t filter) {
        const bool wasEnabled = static_cast<bool>(oldMask & eventMask);
        const bool enabled    = static_cast<bool>(mask & eventMask);
        if (!wasEnabled && !enabled)
            return;
        if (enabled)
            EV_SET(&changes[count++], fd, filter, EV_ADD | EV_ENABLE | EV_DISPATCH, 0, 0, sourceToken(sourceID));
        else
            EV_SET(&changes[count++], fd, filter, EV_DELETE, 0, 0, KqueueUserData{});
    };

    updateFilter(eEventMask::READABLE, EVFILT_READ);
    updateFilter(eEventMask::WRITABLE, EVFILT_WRITE);

    if (count > 0 && kevent(m_state->kqueueFD.get(), changes.data(), count, nullptr, 0, nullptr) < 0) {
        auto error = systemError("kevent(UPDATE)");
        fail(*m_state, error);
        return std::unexpected(std::move(error));
    }

    registrationIt->second.mask = mask;
    return {};
}

void CEventLoopBackend::removeFD(uintptr_t sourceID, int fd) {
    const auto registrationIt = m_state->registrations.find(sourceID);
    if (registrationIt == m_state->registrations.end() || registrationIt->second.fd != fd)
        return;

    std::array<struct kevent, 2> changes;
    int                          count = 0;
    if (registrationIt->second.mask & eEventMask::READABLE)
        EV_SET(&changes[count++], fd, EVFILT_READ, EV_DELETE, 0, 0, KqueueUserData{});
    if (registrationIt->second.mask & eEventMask::WRITABLE)
        EV_SET(&changes[count++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, KqueueUserData{});

    if (count > 0 && kevent(m_state->kqueueFD.get(), changes.data(), count, nullptr, 0, nullptr) < 0 && errno != ENOENT)
        fail(*m_state, systemError("kevent(DELETE)"));
    m_state->registrations.erase(registrationIt);
}

std::expected<void, std::string> CEventLoopBackend::rearmFD(uintptr_t sourceID, int fd, FdEventMask mask) {
    {
        std::lock_guard lock(m_state->pendingMutex);
        if (m_state->failure)
            return std::unexpected(*m_state->failure);
    }

    const auto registrationIt = m_state->registrations.find(sourceID);
    if (registrationIt == m_state->registrations.end() || registrationIt->second.fd != fd)
        return {};

    std::array<struct kevent, 2> changes;
    int                          count = 0;
    if (registrationIt->second.mask & eEventMask::READABLE)
        EV_SET(&changes[count++], fd, EVFILT_READ, EV_ENABLE, 0, 0, sourceToken(sourceID));
    if (registrationIt->second.mask & eEventMask::WRITABLE)
        EV_SET(&changes[count++], fd, EVFILT_WRITE, EV_ENABLE, 0, 0, sourceToken(sourceID));

    if (count > 0 && kevent(m_state->kqueueFD.get(), changes.data(), count, nullptr, 0, nullptr) < 0) {
        auto error = systemError("kevent(ENABLE)");
        fail(*m_state, error);
        return std::unexpected(std::move(error));
    }
    return {};
}

std::expected<void, std::string> CEventLoopBackend::armTimer(std::optional<std::chrono::steady_clock::time_point> deadline) {
    {
        std::lock_guard lock(m_state->pendingMutex);
        if (m_state->failure)
            return std::unexpected(*m_state->failure);
    }

    struct kevent timerEvent;
    if (!deadline) {
        if (!m_state->timerArmed)
            return {};
        EV_SET(&timerEvent, TIMER_IDENT, EVFILT_TIMER, EV_DELETE, 0, 0, KqueueUserData{});
        if (kevent(m_state->kqueueFD.get(), &timerEvent, 1, nullptr, 0, nullptr) < 0 && errno != ENOENT) {
            auto error = systemError("kevent(DELETE timer)");
            fail(*m_state, error);
            return std::unexpected(std::move(error));
        }
        m_state->timerArmed = false;
        return {};
    }

    auto remaining = *deadline - std::chrono::steady_clock::now();
    if (remaining <= std::chrono::steady_clock::duration::zero())
        remaining = std::chrono::nanoseconds{1};

    intptr_t data   = 1;
    uint32_t fflags = 0;
#if defined(NOTE_NSECONDS)
    data   = std::min<int64_t>(std::chrono::ceil<std::chrono::nanoseconds>(remaining).count(), std::numeric_limits<intptr_t>::max());
    fflags = NOTE_NSECONDS;
#elif defined(NOTE_USECONDS)
    data   = std::min<int64_t>(std::chrono::ceil<std::chrono::microseconds>(remaining).count(), std::numeric_limits<intptr_t>::max());
    fflags = NOTE_USECONDS;
#else
    data = std::min<int64_t>(std::chrono::ceil<std::chrono::milliseconds>(remaining).count(), std::numeric_limits<intptr_t>::max());
#endif

    EV_SET(&timerEvent, TIMER_IDENT, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_ONESHOT, fflags, data, KqueueUserData{});
    if (kevent(m_state->kqueueFD.get(), &timerEvent, 1, nullptr, 0, nullptr) < 0) {
        auto error = systemError("kevent(ADD timer)");
        fail(*m_state, error);
        return std::unexpected(std::move(error));
    }

    m_state->timerArmed = true;
    return {};
}

void CEventLoopBackend::signalWake() {
    if (m_state->stopping)
        return;

    {
        std::lock_guard lock(m_state->pendingMutex);
        m_state->wakeRequested = true;
    }
    signalPipe(m_state->controlWriteFD.get());
}

std::expected<std::vector<SBackendEvent>, std::string> CEventLoopBackend::wait(int timeout, size_t maximumEvents) {
    if (timeout == 0) {
        uint64_t syncGeneration = 0;
        {
            std::lock_guard lock(m_state->pendingMutex);
            if (m_state->failure)
                return std::unexpected(*m_state->failure);
            syncGeneration = ++m_state->syncRequested;
        }
        signalPipe(m_state->controlWriteFD.get());

        std::unique_lock lock(m_state->pendingMutex);
        m_state->syncCV.wait(lock, [&] { return m_state->syncCompleted >= syncGeneration || m_state->failure || m_state->stopping; });
        if (m_state->failure)
            return std::unexpected(*m_state->failure);
    }

    {
        std::lock_guard lock(m_state->pendingMutex);
        if (m_state->failure)
            return std::unexpected(*m_state->failure);
    }

    pollfd aggregateFD = {
        .fd      = m_state->aggregateReadFD.get(),
        .events  = POLLIN,
        .revents = 0,
    };

    int result = 0;
    while ((result = poll(&aggregateFD, 1, timeout)) < 0) {
        if (errno == EINTR)
            continue;
        return std::unexpected(systemError("poll(kqueue aggregate)"));
    }
    if (result == 0)
        return std::vector<SBackendEvent>{};

    drainPipe(m_state->aggregateReadFD.get());

    std::vector<SBackendEvent> events;
    bool                       hasRemaining = false;
    {
        std::lock_guard lock(m_state->pendingMutex);
        if (m_state->failure)
            return std::unexpected(*m_state->failure);

        events.reserve(std::min(maximumEvents, m_state->pendingFDs.size() + static_cast<size_t>(m_state->timerPending) + static_cast<size_t>(m_state->wakePending)));
        if (m_state->timerPending && events.size() < maximumEvents) {
            events.push_back({.type = eBackendEventType::TIMER});
            m_state->timerPending = false;
        }
        if (m_state->wakePending && events.size() < maximumEvents) {
            events.push_back({.type = eBackendEventType::WAKE});
            m_state->wakePending = false;
        }

        for (auto eventIt = m_state->pendingFDs.begin(); eventIt != m_state->pendingFDs.end() && events.size() < maximumEvents;) {
            const auto registrationIt = m_state->registrations.find(eventIt->first);
            if (registrationIt != m_state->registrations.end() && registrationIt->second.mask) {
                const auto reported = eventIt->second & (registrationIt->second.mask | eEventMask::HUP | eEventMask::ERROR);
                if (reported)
                    events.push_back({.type = eBackendEventType::FD, .sourceID = eventIt->first, .events = reported});
            }
            eventIt = m_state->pendingFDs.erase(eventIt);
        }

        hasRemaining = m_state->timerPending || m_state->wakePending || !m_state->pendingFDs.empty();
    }

    if (hasRemaining)
        notify(*m_state);
    return events;
}
