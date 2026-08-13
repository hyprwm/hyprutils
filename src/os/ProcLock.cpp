#include <hyprutils/os/ProcLock.hpp>
#include <hyprutils/os/Semaphore.hpp>
#include <hyprutils/string/Numeric.hpp>

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>
#include <system_error>
#include <unistd.h>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <expected>
#include <format>
#include <vector>
#include <chrono>
#include <random>

using namespace Hyprutils;
using namespace Hyprutils::OS;

constexpr const char* LOCK_SUBDIR = ".hyprlocks";

static constexpr bool operator&(CProcLock::eProcLockFlags lhs, CProcLock::eProcLockFlags rhs) noexcept {
    using U = std::underlying_type_t<CProcLock::eProcLockFlags>;
    return (static_cast<U>(lhs) & static_cast<U>(rhs)) != 0;
}

static bool processAlive(pid_t pid) {
    if (pid <= 0)
        return false;

    if (::kill(pid, 0) == 0)
        return true;

    if (errno == EPERM)
        return true;

    if (errno == ESRCH)
        return false;

    return false;
}

static uint32_t randomNumber() {
    std::random_device                                       dev;
    std::mt19937                                             rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, std::numeric_limits<uint32_t>::max());

    return dist(rng);
}

CProcLock::CProcLock(std::string&& instanceName, std::unordered_map<std::string, std::string>&& properties) :
    m_instanceName(std::move(instanceName)), m_properties(std::move(properties)) {
    ;
}

CProcLock::~CProcLock() {
    releaseLock();
}

static std::optional<std::filesystem::path> getLocksDir() {
    const auto XDG_RUNTIME_DIR = getenv("XDG_RUNTIME_DIR");
    if (!XDG_RUNTIME_DIR || XDG_RUNTIME_DIR[0] == 0)
        return std::nullopt;

    const std::string_view XDGRD = XDG_RUNTIME_DIR;

    return std::filesystem::path{XDGRD} / LOCK_SUBDIR;
}

static std::expected<std::filesystem::path, CProcLock::eProcLockObtainingError> writeLockFile(const std::string&                                  instance,
                                                                                              const std::unordered_map<std::string, std::string>& props) {
    const auto LOCKS_DIR = getLocksDir();
    if (!LOCKS_DIR)
        return std::unexpected(CProcLock::eProcLockObtainingError::NO_ENVIRONMENT);

    std::error_code ec;
    if (!std::filesystem::exists(*LOCKS_DIR, ec) || ec) {
        std::filesystem::create_directories(*LOCKS_DIR, ec);

        if (ec)
            return std::unexpected(CProcLock::eProcLockObtainingError::PERMISSIONS_INSUFFICIENT);
    }

    if (!std::filesystem::exists(*LOCKS_DIR, ec) || ec)
        return std::unexpected(CProcLock::eProcLockObtainingError::PERMISSIONS_INSUFFICIENT);

    int        pid = getpid();
    const auto LOCKFILE_NAME =
        std::format(".lock-{}-{}-{}", pid, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), randomNumber());

    auto          lockFilePath = *LOCKS_DIR / LOCKFILE_NAME;
    std::ofstream ofs(lockFilePath, std::ios::trunc);
    if (!ofs.good())
        return std::unexpected(CProcLock::eProcLockObtainingError::PERMISSIONS_INSUFFICIENT);

    auto writeProp = [&ofs](const std::string_view k, const auto v) -> void { ofs << k << "=" << v << "\n"; };

    writeProp("pid", pid);
    writeProp("instance", instance);
    for (const auto& [k, v] : props) {
        writeProp(k, v);
    }

    ofs.close();
    if (!ofs) {
        std::filesystem::remove(lockFilePath, ec);
        return std::unexpected(CProcLock::eProcLockObtainingError::PERMISSIONS_INSUFFICIENT);
    }

    return lockFilePath;
}

static std::optional<CProcLock::SProcLockData> readLockFile(std::filesystem::path path, std::optional<std::string> instance = std::nullopt) {
    CProcLock::SProcLockData data;

    std::ifstream            ifs(path);
    if (!ifs.good())
        return std::nullopt;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.starts_with("pid=")) {
            auto pid = String::strToNumber<int>(line.substr(4));
            if (!pid)
                return std::nullopt;
            data.pid = *pid;

            if (!processAlive(*pid)) {
                std::error_code ec;
                std::filesystem::remove(path, ec);
                return std::nullopt;
            }

            continue;
        }

        if (line.starts_with("instance=")) {
            std::string_view ins = std::string_view{line}.substr(9);
            if (instance.has_value() && ins != *instance)
                return std::nullopt;
            data.instance = ins;
            continue;
        }

        auto equalsPos = line.find('=');
        if (equalsPos == std::string::npos)
            continue;

        auto k = line.substr(0, equalsPos);
        auto v = line.substr(equalsPos + 1);

        data.m_properties.emplace(std::move(k), std::move(v));
    }

    if (data.instance.empty() || data.pid <= 0)
        return std::nullopt;

    if (instance && data.instance != *instance)
        return std::nullopt;

    return data;
};

static std::vector<CProcLock::SProcLockData> enumerateInternal(const std::string& inst) {
    std::vector<CProcLock::SProcLockData> result;

    const auto                            LOCKS_DIR = getLocksDir();
    if (!LOCKS_DIR)
        return result;

    std::error_code ec;
    for (const auto& f : std::filesystem::directory_iterator(*LOCKS_DIR, ec)) {
        if (!f.is_regular_file())
            continue;

        auto res = readLockFile(f, inst);
        if (!res)
            continue;

        result.emplace_back(std::move(*res));
    }

    return result;
}

std::expected<void, CProcLock::eProcLockObtainingError> CProcLock::obtain(eProcLockFlags flags) {
#define WRITE_OR_ERR()                                                                                                                                                             \
    {                                                                                                                                                                              \
        auto r = writeLockFile(m_instanceName, m_properties);                                                                                                                      \
        if (!r)                                                                                                                                                                    \
            return std::unexpected(r.error());                                                                                                                                     \
        m_lockPath = *r;                                                                                                                                                           \
        m_ownerPid = getpid();                                                                                                                                                     \
        return {};                                                                                                                                                                 \
    }

    auto sem = CSemaphore("hyprutils_ProcLock");

    if (!m_lockPath.empty())
        return std::unexpected(eProcLockObtainingError::ALREADY_TAKEN);

    if (flags == eProcLockFlags::NO_FLAGS)
        WRITE_OR_ERR();

    if (flags & eProcLockFlags::EXCLUSIVE) {
        const auto enumerated = enumerateInternal(m_instanceName);

        if (enumerated.size() > 0) {
            for (const auto& e : enumerated) {
                bool propertiesDiffer = false;
                for (const auto& [k, v] : m_properties) {
                    if (!std::ranges::any_of(e.m_properties, [&k, &v](const auto& property) { return property.first == k && property.second == v; })) {
                        propertiesDiffer = true;
                        break;
                    }
                }

                if (!propertiesDiffer)
                    return std::unexpected(CProcLock::eProcLockObtainingError::ALREADY_RUNNING);
            }
        }
    }

    WRITE_OR_ERR();

#undef WRITE_OR_ERR
}

std::vector<CProcLock::SProcLockData> CProcLock::enumerate() const {
    auto sem = CSemaphore("hyprutils_ProcLock");
    return enumerateInternal(m_instanceName);
}

void CProcLock::releaseLock() const {
    if (m_lockPath.empty() || m_ownerPid != getpid())
        return;

    auto sem = CSemaphore("hyprutils_ProcLock");

    //
    std::error_code ec;
    std::filesystem::remove(m_lockPath, ec);
}
