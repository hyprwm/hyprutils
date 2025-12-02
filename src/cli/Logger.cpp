#include "Logger.hpp"

#include <chrono>
#include <print>

using namespace Hyprutils;
using namespace Hyprutils::CLI;

CLogger::CLogger() {
    m_impl = Memory::makeUnique<CLoggerImpl>(this);
}

CLogger::~CLogger() = default;

void CLogger::setLogLevel(eLogLevel level) {
    m_logLevel = level;
}

void CLogger::setTime(bool enabled) {
    m_impl->m_timeEnabled = enabled;
}

void CLogger::setEnableStdout(bool enabled) {
    m_impl->m_stdoutEnabled = enabled;
    m_impl->updateParentShouldLog();
}

void CLogger::setEnableColor(bool enabled) {
    m_impl->m_colorEnabled = enabled;
}

void CLogger::setEnableRolling(bool enabled) {
    m_impl->m_rollingEnabled = enabled;
}

std::expected<void, std::string> CLogger::setOutputFile(const std::string_view& file) {
    if (file.empty()) {
        m_impl->m_fileEnabled = false;
        m_impl->m_logOfs      = {};
        return {};
    }

    std::filesystem::path filePath{file};
    std::error_code       ec;

    if (!filePath.has_parent_path())
        return std::unexpected("Path has no parent");

    auto dir = filePath.parent_path();

    if (!std::filesystem::exists(dir, ec) || ec)
        return std::unexpected("Parent path is inaccessible, or doesn't exist");

    m_impl->m_logOfs      = std::ofstream{filePath, std::ios::trunc};
    m_impl->m_logFilePath = filePath;

    if (!m_impl->m_logOfs.good())
        return std::unexpected("Failed to open a write stream");

    m_impl->m_fileEnabled = true;
    m_impl->updateParentShouldLog();

    return {};
}

void CLogger::log(eLogLevel level, const std::string_view& msg) {
    if (!m_shouldLogAtAll)
        return;

    if (level < m_logLevel)
        return;

    m_impl->log(level, msg);
}

const std::string& CLogger::rollingLog() {
    return m_impl->m_rollingLog;
}

CLoggerImpl::CLoggerImpl(CLogger* parent) : m_parent(parent) {
    updateParentShouldLog();
}

void CLoggerImpl::log(eLogLevel level, const std::string_view& msg, const std::string_view& from) {
    std::lock_guard<std::mutex> lg(m_logMtx);

    std::string                 logPrefix = "", logPrefixColor = "";
    std::string                 logMsg = "";

    switch (level) {
        case LOG_TRACE:
            logPrefix += "TRACE ";
            logPrefixColor += "\033[1;34mTRACE \033[0m";
            break;
        case LOG_DEBUG:
            logPrefix += "DEBUG ";
            logPrefixColor += "\033[1;32mDEBUG \033[0m";
            break;
        case LOG_WARN:
            logPrefix += "WARN ";
            logPrefixColor += "\033[1;33mWARN \033[0m";
            break;
        case LOG_ERR:
            logPrefix += "ERR ";
            logPrefixColor += "\033[1;31mERR \033[0m";
            break;
        case LOG_CRIT:
            logPrefix += "CRIT ";
            logPrefixColor += "\033[1;35mCRIT \033[0m";
            break;
    }

    if (m_timeEnabled) {
#ifndef _LIBCPP_VERSION
        static auto current_zone = std::chrono::current_zone();
        const auto  zt           = std::chrono::zoned_time{current_zone, std::chrono::system_clock::now()};
        const auto  hms          = std::chrono::hh_mm_ss{zt.get_local_time() - std::chrono::floor<std::chrono::days>(zt.get_local_time())};
#else
        // TODO: current clang 17 does not support `zoned_time`, remove this once clang 19 is ready
        const auto hms = std::chrono::hh_mm_ss{std::chrono::system_clock::now() - std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
#endif
        logMsg += std::format("@ {} ", hms);
    }

    if (!from.empty()) {
        logMsg += "from ";
        logMsg += from;
        logMsg += " ";
    }

    logMsg += "]: ";
    logMsg += msg;

    if (m_stdoutEnabled) {
        try {
            std::println("{}{}", m_colorEnabled ? logPrefixColor : logPrefix, logMsg);
        } catch (std::exception& e) {
            ; // this could be e.g. stdout closed
        }
    }
    if (m_fileEnabled)
        m_logOfs << logPrefix << logMsg << "\n";

    if (m_rollingEnabled)
        appendToRolling(logPrefix + logMsg);
}

void CLoggerImpl::updateParentShouldLog() {
    m_parent->m_shouldLogAtAll = m_fileEnabled || m_stdoutEnabled;
}

void CLoggerImpl::appendToRolling(const std::string& s) {
    constexpr const size_t ROLLING_LOG_SIZE = 4096;
    if (!m_rollingLog.empty())
        m_rollingLog += "\n";
    m_rollingLog += s;
    if (m_rollingLog.size() > ROLLING_LOG_SIZE)
        m_rollingLog = m_rollingLog.substr(m_rollingLog.find('\n', m_rollingLog.size() - ROLLING_LOG_SIZE) + 1);
}

CLoggerConnection::CLoggerConnection(CLogger& logger) : m_impl(logger.m_impl), m_logger(&logger), m_logLevel(logger.m_logLevel) {
    ;
}

CLoggerConnection::~CLoggerConnection() = default;

void CLoggerConnection::setName(const std::string_view& name) {
    m_name = name;
}

void CLoggerConnection::setLogLevel(eLogLevel level) {
    m_logLevel = level;
}

void CLoggerConnection::log(eLogLevel level, const std::string_view& msg) {
    if (!m_impl || !m_logger)
        return;

    if (!m_logger->m_shouldLogAtAll)
        return;

    if (level < m_logLevel)
        return;

    m_impl->log(level, msg, m_name);
}

CLogger* CLoggerConnection::getLogger() {
    if (!m_impl)
        return nullptr;

    return m_logger;
}

void CLoggerConnection::redirect(CLogger& logger) {
    m_impl   = logger.m_impl;
    m_logger = &logger;
}
