#include <hyprutils/path/Path.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/debug/Log.hpp>
#include <filesystem>

using namespace Hyprutils;

namespace Hyprutils::Path {
    std::string fullConfigPath(std::string basePath, std::string programName) {
        return basePath + "/hypr/" + programName + ".conf";
    }

    bool checkConfigExists(std::string basePath, std::string programName) {
        return std::filesystem::exists(fullConfigPath(basePath, programName));
    }

    std::optional<std::string> getHome() {
        static const auto homeDir = getenv("HOME");

        if (!homeDir || !std::filesystem::path(homeDir).is_absolute()) {
            Debug::log(INFO, "'$HOME' is either not found or not an absolute path");
            return std::nullopt;
        }

        return std::string(homeDir).append("/.config");
    }

    std::optional<String::CVarList> getXdgConfigDirs() {
        static const auto xdgConfigDirs = getenv("XDG_CONFIG_DIRS");

        if (!xdgConfigDirs) {
            Debug::log(INFO, "'$XDG_CONFIG_DIRS' is not set");
            return std::nullopt;
        }

        static const auto xdgConfigDirsList = String::CVarList(xdgConfigDirs, 0, ':');

        return xdgConfigDirsList;
    }

    std::optional<std::string> getXdgConfigHome() {
        static const auto xdgConfigHome = getenv("XDG_CONFIG_HOME");

        if (!xdgConfigHome || !std::filesystem::path(xdgConfigHome).is_absolute()) {
            Debug::log(INFO, "'$XDG_CONFIG_HOME' is either not found or not an absolute path");
            return std::nullopt;
        }

        return xdgConfigHome;
    }

    using T = std::optional<std::string>;
    std::pair<T, T> findConfig(std::string programName) {
        std::string       configPath;

        bool              xdgConfigHomeExists = false;
        static const auto xdgConfigHome       = getXdgConfigHome();
        if (xdgConfigHome.has_value()) {
            xdgConfigHomeExists = true;
            configPath = fullConfigPath(xdgConfigHome.value(), programName);
            if (std::filesystem::exists(configPath))
                return std::make_pair(configPath, xdgConfigHome);
            Debug::log(INFO, "No config file {}", configPath);
        }

        bool              homeExists = false;
        static const auto home       = getHome();
        if (home.has_value()) {
            homeExists = true;
            configPath = fullConfigPath(home.value(), programName);
            if (std::filesystem::exists(configPath))
                return std::make_pair(configPath, home);
            Debug::log(INFO, "No config file {}", configPath);
        }

        static const auto xdgConfigDirs = getXdgConfigDirs();
        if (xdgConfigDirs.has_value()) {
            for (auto dir : xdgConfigDirs.value()) {
                configPath = fullConfigPath(dir, programName);
                if (std::filesystem::exists(configPath))
                    return std::make_pair(configPath, std::nullopt);
                Debug::log(INFO, "No config file {}", configPath);
            }
        }

        configPath = fullConfigPath("/etc/xdg", programName);
        if (std::filesystem::exists(configPath))
            return std::make_pair(configPath, std::nullopt);
        Debug::log(INFO, "No config file {}", configPath);

        if (xdgConfigHomeExists)
            return std::make_pair(std::nullopt, xdgConfigHome);
        else if (homeExists)
            return std::make_pair(std::nullopt, home);

        Debug::log(ERR, "No config file could be found. Check previous logs for clues");
        return std::make_pair(std::nullopt, std::nullopt);
    }
}
