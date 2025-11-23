#pragma once

#include <format>
#include <expected>
#include <string_view>

#include "../memory/UniquePtr.hpp"

namespace Hyprutils::CLI {
    class CLoggerImpl;

    enum eLogLevel : uint8_t {
        LOG_TRACE = 0,
        LOG_DEBUG,
        LOG_WARN,
        LOG_ERR,
        LOG_CRIT,
    };

    // CLogger is a thread-safe, general purpose logger.
    // the logger's stdout is enabled by default.
    // color is enabled by default, it's only for stdout.
    // everything else is disabled.
    class CLogger {
      public:
        CLogger();
        ~CLogger();

        CLogger(const CLogger&) = delete;
        CLogger(CLogger&)       = delete;
        CLogger(CLogger&&)      = delete;

        void                             setTrace(bool enabled);
        void                             setTime(bool enabled);
        void                             setEnableStdout(bool enabled);
        void                             setEnableColor(bool enabled);
        std::expected<void, std::string> setOutputFile(const std::string_view& file);
        const std::string&               rollingLog();

        void                             log(eLogLevel level, const std::string_view& msg);

        template <typename... Args>
        // NOLINTNEXTLINE
        void log(eLogLevel level, std::format_string<Args...> fmt, Args&&... args) {
            if (!m_shouldLogAtAll)
                return;

            if (level == LOG_TRACE && !m_trace)
                return;

            std::string logMsg = std::vformat(fmt.get(), std::make_format_args(args...));
            log(level, logMsg);
        }

      private:
        Memory::CUniquePointer<CLoggerImpl> m_impl;

        // this has to be here as part of important optimization of trace logs
        bool m_trace = false;

        // this has to be here as part of important optimization of disabled logging
        bool m_shouldLogAtAll = false;

        friend class CLoggerImpl;
    };
};