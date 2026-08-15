#pragma once

#include <hyprutils/eventLoop/EventMask.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace Hyprutils::EventLoop {
    enum class eBackendEventType : uint8_t {
        FD,
        TIMER,
        WAKE,
    };

    struct SBackendEvent {
        eBackendEventType type;
        uintptr_t         sourceID = 0;
        FdEventMask       events   = eEventMask::EMPTY;
    };

    class CEventLoopBackend {
      public:
        static std::expected<Memory::CUniquePointer<CEventLoopBackend>, std::string> create();

        ~CEventLoopBackend();

        CEventLoopBackend(const CEventLoopBackend&)                                                = delete;
        CEventLoopBackend(CEventLoopBackend&&)                                                     = delete;
        CEventLoopBackend&                                     operator=(const CEventLoopBackend&) = delete;
        CEventLoopBackend&                                     operator=(CEventLoopBackend&&)      = delete;

        int                                                    fd() const;

        std::expected<void, std::string>                       addFD(uintptr_t sourceID, int fd, FdEventMask mask);
        std::expected<void, std::string>                       updateFD(uintptr_t sourceID, int fd, FdEventMask mask);
        void                                                   removeFD(uintptr_t sourceID, int fd);
        std::expected<void, std::string>                       rearmFD(uintptr_t sourceID, int fd, FdEventMask mask);

        std::expected<void, std::string>                       armTimer(std::optional<std::chrono::steady_clock::time_point> deadline);
        void                                                   signalWake();

        std::expected<std::vector<SBackendEvent>, std::string> wait(int timeout, size_t maximumEvents);

      private:
        CEventLoopBackend() = default;

        struct SBackendState;
        static void                           notify(SBackendState& state);
        static void                           fail(SBackendState& state, std::string error);
        static void                           runWorker(SBackendState* state);

        Memory::CUniquePointer<SBackendState> m_state;
    };
}
