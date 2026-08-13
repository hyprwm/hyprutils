#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <expected>

namespace Hyprutils::OS {

    /*
     * A process lock allows an app to control single-instance, or discovery, based on the filesystem,
     * which means it does not rely on any session / system bus.
     * Locks operate on PIDs, namespaces and additional properties. Namespaces define what app we are.
     * Additional properties can be e.g. WAYLAND_DISPLAY to not enforce single-instance across display servers.
     * For example, a wallpaper daemon might refuse to spawn another instance, but should still be able to on
     * another WAYLAND_DISPLAY.
     *
     * Releasing this lock (calling the ~dtor) will release the lock!
     */
    class CProcLock {
      public:
        /*
         * Additional lock flags for obtaining the lock.
         */
        enum class eProcLockFlags : uint8_t {
            NO_FLAGS,

            // This session MUST NOT match any other existing lock
            EXCLUSIVE,
        };

        CProcLock(std::string&& instanceName, std::unordered_map<std::string, std::string>&& properties);
        ~CProcLock();

        CProcLock(const CProcLock&)            = delete;
        CProcLock& operator=(const CProcLock&) = delete;
        CProcLock(CProcLock&&)                 = delete;
        CProcLock& operator=(CProcLock&&)      = delete;

        /*
         * Attempt to obtain the lock. If EXCLUSIVE is set, this will return an error if another
         * running lock matches this lock.
         */
        enum class eProcLockObtainingError : uint8_t {
            UNKNOWN,
            // lock aleady exists
            ALREADY_TAKEN,
            // XDG_RUNTIME_DIR is unset
            NO_ENVIRONMENT,
            // an instance is already running, and we are in exclusive
            ALREADY_RUNNING,
            // permissions insufficient to create locks
            PERMISSIONS_INSUFFICIENT
        };
        std::expected<void, eProcLockObtainingError> obtain(eProcLockFlags flags);

        struct SProcLockData {
            std::string                                  instance = "";
            int                                          pid      = -1;
            std::unordered_map<std::string, std::string> m_properties;
        };

        /*
         * Enumerate running locks of the same instance. This function also cleans up stale locks
         * which might have been left by crashed applications (stale PID)
         */
        std::vector<SProcLockData> enumerate() const;

      private:
        std::string                                  m_instanceName;
        std::unordered_map<std::string, std::string> m_properties;
        std::filesystem::path                        m_lockPath;
        int                                          m_ownerPid = -1;

        void                                         releaseLock() const;
    };
};
