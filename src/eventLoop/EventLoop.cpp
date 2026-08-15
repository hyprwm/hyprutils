#include "EventLoop.hpp"

#include "FDSource.hpp"
#include "PostDispatchHook.hpp"
#include "Timer.hpp"

#include <hyprutils/utils/ScopeGuard.hpp>

#include <algorithm>
#include <utility>

using namespace Hyprutils::EventLoop;
using namespace Hyprutils::Memory;
using namespace Hyprutils::OS;
using namespace Hyprutils::Utils;

// A broken or permanently ready level-triggered source must not starve idles,
// post-dispatch hooks, or executor work forever.
static constexpr size_t                 MAX_EVENTS_PER_DISPATCH = 256;

static std::expected<void, std::string> validateMask(FdEventMask mask) {
    if ((mask & eEventMask::HUP) || (mask & eEventMask::ERROR))
        return std::unexpected("HUP and ERROR are reported events and cannot be requested");
    return {};
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
    auto backend = CEventLoopBackend::create();
    if (!backend)
        return std::unexpected(backend.error());
    m_backend = std::move(*backend);

    m_executorState       = makeAtomicShared<SExecutorState>();
    m_executorState->wake = [backend = m_backend.get()] { backend->signalWake(); };
    m_executor            = CAtomicSharedPointer<ILoopExecutor>{new CLoopExecutor{m_executorState}};
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
    if (auto result = validateMask(mask); !result)
        return std::unexpected(result.error());

    const auto id     = m_nextID++;
    auto       source = makeShared<CFDSource>(*this, id, std::move(fd), mask, std::move(callback));
    if (auto result = m_backend->addFD(id, source->fd().get(), mask); !result)
        return std::unexpected(result.error());

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
    return m_backend ? m_backend->fd() : -1;
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

    if (auto result = drainReady(timeout); !result)
        return result;
    if (m_pendingError)
        return std::unexpected(*m_pendingError);

    dispatchIdles();
    dispatchPostHooks();
    return {};
}

std::expected<void, std::string> CEventLoop::drainReady(int timeout) {
    size_t dispatched = 0;

    while (true) {
        auto events = m_backend->wait(timeout, MAX_EVENTS_PER_DISPATCH - dispatched);
        if (!events)
            return std::unexpected(events.error());
        if (events->empty())
            return {};

        timeout = 0;
        for (const auto& event : *events) {
            if (event.type == eBackendEventType::TIMER)
                dispatchTimers();
            else if (event.type == eBackendEventType::WAKE)
                drainExecutor();
            else {
                const auto sourceIt = m_sources.find(event.sourceID);
                if (sourceIt == m_sources.end())
                    continue;

                auto source = sourceIt->second;
                if (!source->mask())
                    continue;

                const auto reported = event.events & (source->mask() | eEventMask::HUP | eEventMask::ERROR);
                {
                    CScopeGuard rearmGuard([this, id = event.sourceID, source] {
                        const auto current = m_sources.find(id);

                        if (current == m_sources.end() || current->second != source)
                            return;

                        if (auto result = m_backend->rearmFD(id, source->fd().get(), source->mask()); !result) {
                            m_pendingError = result.error();
                            wakeIdle();
                        }
                    });

                    if (reported)
                        source->call(reported);
                }
            }

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

    if (auto result = validateMask(mask); !result)
        return result;

    return m_backend->updateFD(source.id(), source.fd().get(), mask);
}

void CEventLoop::removeSource(CFDSource& source) {
    const auto sourceIt = m_sources.find(source.id());
    if (sourceIt == m_sources.end() || sourceIt->second.get() != &source)
        return;

    auto keepAlive = sourceIt->second;
    m_backend->removeFD(source.id(), source.fd().get());
    m_sources.erase(sourceIt);
    keepAlive->detach();
}

void CEventLoop::timerChanged(bool cleanup) {
    if (!m_backend)
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

    if (auto result = m_backend->armTimer(next); !result) {
        m_pendingError = result.error();
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
    for (auto& idle : idles) {
        idle();
    }
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
    std::deque<std::function<void()>> callbacks;
    {
        std::lock_guard lock(m_executorState->mutex);
        callbacks.swap(m_executorState->callbacks);
    }

    for (auto& callback : callbacks)
        addIdle(std::move(callback));
}

void CEventLoop::wakeIdle() {
    if (m_backend)
        m_backend->signalWake();
}

void CEventLoop::detachAll() {
    m_running = false;

    std::deque<std::function<void()>> discardedCallbacks;
    if (m_executorState) {
        std::lock_guard lock(m_executorState->mutex);
        m_executorState->active = false;
        discardedCallbacks.swap(m_executorState->callbacks);
        m_executorState->wake = {};
    }

    for (const auto& weakTimer : m_timers) {
        if (auto timer = weakTimer.lock())
            timer->detach();
    }

    m_backend.reset();

    for (auto& [id, source] : m_sources)
        source->detach();

    m_sources.clear();
    m_timers.clear();
    m_postHooks.clear();
    m_idles.clear();
    m_executor.reset();
    m_executorState.reset();
}
