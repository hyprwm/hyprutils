#include "EventLoop.hpp"

#include "FDSource.hpp"
#include "PostDispatchHook.hpp"
#include "Timer.hpp"

#include <hyprutils/utils/ScopeGuard.hpp>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <utility>

using namespace Hyprutils::EventLoop;
using namespace Hyprutils::Memory;
using namespace Hyprutils::OS;
using namespace Hyprutils::Utils;

// A broken or permanently ready level-triggered source must not starve idles,
// post-dispatch hooks, or executor work forever.
static constexpr size_t MAX_EVENTS_PER_DISPATCH = 256;

static std::string      systemError(const std::string& operation) {
    return operation + ": " + std::strerror(errno);
}

static std::expected<uint32_t, std::string> epollMask(FdEventMask mask) {
    if ((mask & eEventMask::HUP) || (mask & eEventMask::ERROR))
        return std::unexpected("HUP and ERROR are reported events and cannot be requested");

    uint32_t events = 0;
    if (mask & eEventMask::READABLE)
        events |= EPOLLIN;
    if (mask & eEventMask::WRITABLE)
        events |= EPOLLOUT;
    events |= EPOLLRDHUP;
    return events;
}

static FdEventMask eventMask(uint32_t events) {
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

static void signalEventFD(int fd) {
    const uint64_t value = 1;
    while (write(fd, &value, sizeof(value)) < 0) {
        if (errno == EINTR)
            continue;
        return;
    }
}

std::expected<CSharedPointer<IEventLoop>, std::string> IEventLoop::create() {
    auto loop = makeShared<CEventLoop>();
    loop->setSelf(loop);
    if (auto result = loop->init(); !result)
        return std::unexpected(result.error());
    return CSharedPointer<IEventLoop>{std::move(loop)};
}

CEventLoop::~CEventLoop() {
    detachAll();
}

void CEventLoop::setSelf(const CSharedPointer<CEventLoop>& self) {
    m_self = self;
}

std::expected<void, std::string> CEventLoop::init() {
    m_epollFD = CFileDescriptor{epoll_create1(EPOLL_CLOEXEC)};

    if (!m_epollFD.isValid())
        return std::unexpected(systemError("epoll_create1"));

    CFileDescriptor timerFD{timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK)};

    if (!timerFD.isValid())
        return std::unexpected(systemError("timerfd_create"));

    m_timerFD = timerFD.get();

    auto timerSource = addFDInternal(std::move(timerFD), eEventMask::READABLE, [this](IFDSource&, FdEventMask events) {
        if (!(events & eEventMask::READABLE))
            return;
        drainEventFD(m_timerFD);
        dispatchTimers();
    });
    if (!timerSource)
        return std::unexpected(timerSource.error());

    CFileDescriptor idleFD{eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)};

    if (!idleFD.isValid())
        return std::unexpected(systemError("eventfd"));

    m_idleFD = idleFD.get();

    auto idleSource = addFDInternal(std::move(idleFD), eEventMask::READABLE, [this](IFDSource&, FdEventMask events) {
        if (events & eEventMask::READABLE)
            drainEventFD(m_idleFD);
    });
    if (!idleSource)
        return std::unexpected(idleSource.error());

    m_executorState          = makeAtomicShared<SExecutorState>();
    m_executorState->eventFD = CFileDescriptor{eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)};

    if (!m_executorState->eventFD.isValid())
        return std::unexpected(systemError("eventfd"));

    auto executorFD = m_executorState->eventFD.duplicate();
    if (!executorFD.isValid())
        return std::unexpected(systemError("fcntl"));

    auto executorSource = addFDInternal(std::move(executorFD), eEventMask::READABLE, [this](IFDSource&, FdEventMask events) {
        if (events & eEventMask::READABLE)
            drainExecutor();
    });
    if (!executorSource)
        return std::unexpected(executorSource.error());

    m_executor = CAtomicSharedPointer<ILoopExecutor>{new CLoopExecutor{m_executorState}};
    return {};
}

std::expected<CSharedPointer<IFDSource>, std::string> CEventLoop::addFD(CFileDescriptor&& fd, FdEventMask mask, std::function<void(IFDSource&, FdEventMask)>&& callback) {
    auto source = addFDInternal(std::move(fd), mask, std::move(callback));
    if (!source)
        return std::unexpected(source.error());
    return CSharedPointer<IFDSource>{std::move(*source)};
}

std::expected<CSharedPointer<CFDSource>, std::string> CEventLoop::addFDInternal(CFileDescriptor&& fd, FdEventMask mask, std::function<void(IFDSource&, FdEventMask)>&& callback) {
    if (!fd.isValid())
        return std::unexpected("cannot add an invalid file descriptor");

    if (!callback)
        return std::unexpected("cannot add a file descriptor without a callback");

    const auto events = epollMask(mask);
    if (!events)
        return std::unexpected(events.error());

    const auto  id     = m_nextID++;
    auto        source = makeShared<CFDSource>(*this, id, std::move(fd), mask, std::move(callback));
    epoll_event event  = {
        .events = *events,
        .data   = {.u64 = id},
    };

    if (epoll_ctl(m_epollFD.get(), EPOLL_CTL_ADD, source->fd().get(), &event) < 0)
        return std::unexpected(systemError("epoll_ctl(ADD)"));

    m_sources.emplace(id, source);
    return source;
}

CSharedPointer<ITimer> CEventLoop::addTimer(std::optional<std::chrono::steady_clock::duration> timeout, std::function<void(ITimer&)>&& callback) {
    auto timer = makeShared<CTimer>(*this, timeout, std::move(callback));
    m_timers.emplace_back(timer);
    timerChanged();
    return CSharedPointer<ITimer>{std::move(timer)};
}

void CEventLoop::addIdle(std::function<void()> fn) {
    if (!fn)
        return;

    m_idles.emplace_back(std::move(fn));
    if (!m_dispatching)
        wakeIdle();
}

CSharedPointer<IPostDispatchHook> CEventLoop::addPostDispatch(std::function<void()>&& callback) {
    if (!callback)
        return {};

    auto hook = makeShared<CPostDispatchHook>(std::move(callback));
    m_postHooks.emplace_back(hook);
    return CSharedPointer<IPostDispatchHook>{std::move(hook)};
}

CAtomicSharedPointer<ILoopExecutor> CEventLoop::executor() {
    return m_executor;
}

int CEventLoop::fd() const {
    return m_epollFD.get();
}

std::expected<void, std::string> CEventLoop::dispatch() {
    return dispatchCycle(0);
}

std::expected<void, std::string> CEventLoop::enterLoop() {
    auto keepAlive = m_self.lock();
    if (!keepAlive)
        return std::unexpected("event loop is being destroyed");
    if (m_running)
        return std::unexpected("event loop is already running");

    CScopeGuard runningGuard([this] { m_running = false; });
    m_running = true;
    while (m_running) {
        if (auto result = dispatchCycle(-1); !result)
            return result;
    }

    return {};
}

void CEventLoop::stop() {
    m_running = false;
}

std::expected<void, std::string> CEventLoop::dispatchCycle(int timeout) {
    auto keepAlive = m_self.lock();

    if (!keepAlive)
        return std::unexpected("event loop is being destroyed");

    if (m_dispatching)
        return std::unexpected("event loop dispatch is not reentrant");

    CScopeGuard dispatchGuard([this] {
        m_dispatching = false;
        if (!m_idles.empty())
            wakeIdle();
    });
    m_dispatching = true;

    if (m_pendingError)
        return std::unexpected(*m_pendingError);

    auto result = drainReady(timeout);
    if (!result)
        return result;

    if (m_pendingError)
        return std::unexpected(*m_pendingError);

    dispatchIdles();
    dispatchPostHooks();

    return {};
}

std::expected<void, std::string> CEventLoop::drainReady(int timeout) {
    std::vector<epoll_event> events;
    size_t                   dispatched = 0;

    while (true) {
        events.resize(std::max<size_t>(16, m_sources.size()));

        int count = epoll_wait(m_epollFD.get(), events.data(), events.size(), timeout);
        if (count < 0) {
            if (errno == EINTR)
                continue;
            return std::unexpected(systemError("epoll_wait"));
        }

        if (count == 0)
            return {};

        timeout = 0;
        for (int i = 0; i < count; ++i) {
            const auto sourceIt = m_sources.find(events[i].data.u64);
            if (sourceIt == m_sources.end())
                continue;

            auto source = sourceIt->second;
            source->call(eventMask(events[i].events));

            if (m_pendingError)
                return std::unexpected(*m_pendingError);

            if (++dispatched >= MAX_EVENTS_PER_DISPATCH)
                return {};
        }
    }
}

std::expected<void, std::string> CEventLoop::updateSourceMask(CFDSource& source, FdEventMask mask) {
    const auto sourceIt = m_sources.find(source.id());
    if (sourceIt == m_sources.end() || sourceIt->second.get() != &source)
        return std::unexpected("file descriptor source has been removed");

    const auto events = epollMask(mask);
    if (!events)
        return std::unexpected(events.error());

    epoll_event event = {
        .events = *events,
        .data   = {.u64 = source.id()},
    };

    if (epoll_ctl(m_epollFD.get(), EPOLL_CTL_MOD, source.fd().get(), &event) < 0)
        return std::unexpected(systemError("epoll_ctl(MOD)"));

    return {};
}

void CEventLoop::removeSource(CFDSource& source) {
    const auto sourceIt = m_sources.find(source.id());
    if (sourceIt == m_sources.end() || sourceIt->second.get() != &source)
        return;

    auto keepAlive = sourceIt->second;
    if (m_epollFD.isValid() && source.fd().isValid())
        epoll_ctl(m_epollFD.get(), EPOLL_CTL_DEL, source.fd().get(), nullptr);
    m_sources.erase(sourceIt);
    keepAlive->detach();
}

void CEventLoop::timerChanged(bool cleanup) {
    if (m_timerFD < 0)
        return;

    // A timer destructor calls this while its shared-pointer control block is still
    // active. Do not drop that timer's last weak reference from inside its destructor.
    if (cleanup)
        std::erase_if(m_timers, [](const auto& timer) { return timer.expired(); });

    std::optional<Timestamp> next;
    for (const auto& weakTimer : m_timers) {
        const auto timer = weakTimer.lock();
        if (!timer || !timer->expiresAt())
            continue;
        if (!next || *timer->expiresAt() < *next)
            next = timer->expiresAt();
    }

    itimerspec spec = {};
    if (next) {
        auto remaining = *next - std::chrono::steady_clock::now();
        if (remaining <= Duration::zero())
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

    if (timerfd_settime(m_timerFD, 0, &spec, nullptr) < 0) {
        m_pendingError = systemError("timerfd_settime");
        wakeIdle();
    }
}

void CEventLoop::dispatchTimers() {
    CScopeGuard                         rearmGuard([this] { timerChanged(); });
    const auto                          now = std::chrono::steady_clock::now();
    std::vector<CSharedPointer<CTimer>> expired;

    for (const auto& weakTimer : m_timers) {
        auto timer = weakTimer.lock();
        if (timer && timer->expiresAt() && *timer->expiresAt() <= now)
            expired.emplace_back(std::move(timer));
    }

    for (const auto& timer : expired) {
        if (timer.strongRef() == 1 || !timer->expired())
            continue;
        timer->fire();
    }
}

void CEventLoop::dispatchIdles() {
    auto idles = std::move(m_idles);
    m_idles.clear();

    for (auto& idle : idles)
        idle();
}

void CEventLoop::dispatchPostHooks() {
    std::erase_if(m_postHooks, [](const auto& hook) { return hook.expired(); });

    std::vector<CSharedPointer<CPostDispatchHook>> hooks;
    hooks.reserve(m_postHooks.size());
    for (const auto& weakHook : m_postHooks) {
        if (auto hook = weakHook.lock())
            hooks.emplace_back(std::move(hook));
    }

    for (const auto& hook : hooks) {
        if (hook.strongRef() > 1)
            hook->call();
    }
}

void CEventLoop::drainExecutor() {
    drainEventFD(m_executorState->eventFD.get());

    std::deque<std::function<void()>> callbacks;
    {
        std::lock_guard lock(m_executorState->mutex);
        callbacks.swap(m_executorState->callbacks);
    }

    for (auto& callback : callbacks)
        addIdle(std::move(callback));
}

void CEventLoop::wakeIdle() {
    if (m_idleFD >= 0)
        signalEventFD(m_idleFD);
}

void CEventLoop::drainEventFD(int fd) {
    uint64_t value = 0;
    while (read(fd, &value, sizeof(value)) < 0 && errno == EINTR) {}
}

void CEventLoop::detachAll() {
    m_running = false;

    std::deque<std::function<void()>> discardedCallbacks;
    if (m_executorState) {
        std::lock_guard lock(m_executorState->mutex);
        m_executorState->active = false;
        discardedCallbacks.swap(m_executorState->callbacks);
        m_executorState->eventFD.reset();
    }

    for (const auto& weakTimer : m_timers) {
        if (auto timer = weakTimer.lock())
            timer->detach();
    }

    for (auto& [id, source] : m_sources)
        source->detach();

    m_sources.clear();
    m_timers.clear();
    m_postHooks.clear();
    m_idles.clear();
    m_executor.reset();
    m_executorState.reset();
    m_timerFD = -1;
    m_idleFD  = -1;
    m_epollFD.reset();
}
