#include <hyprutils/path/Path.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/debug/Log.hpp>
#include <filesystem>
#include <set>

using namespace Hyprutils;

namespace Hyprutils::Path {
    std::string fullConfigPath(std::string basePath, std::string programName) {
        return basePath + "/hypr/" + programName + ".conf";
    }

    std::optional<std::string> getHome() {
        static const auto homeDir = getenv("HOME");

        if (!homeDir || !std::filesystem::path(homeDir).is_absolute()) {
            Debug::log(LOG, "'$HOME' is either not found or not an absolute path");
            return std::nullopt;
        }

        return std::string(homeDir).append("/.config");
    }

    std::optional<String::CVarList> getXdgConfigDirs() {
        static const auto xdgConfigDirs = getenv("XDG_CONFIG_DIRS");

        if (!xdgConfigDirs) {
            Debug::log(LOG, "'$XDG_CONFIG_DIRS' is not set");
            return std::nullopt;
        }

        static const auto xdgConfigDirsList = String::CVarList(xdgConfigDirs, 0, ':');

        return xdgConfigDirsList;
    }

    std::optional<std::string> getXdgConfigHome() {
        static const auto xdgConfigHome = getenv("XDG_CONFIG_HOME");

        if (!xdgConfigHome || !std::filesystem::path(xdgConfigHome).is_absolute()) {
            Debug::log(LOG, "'$XDG_CONFIG_HOME' is either not found or not an absolute path");
            return std::nullopt;
        }

        return xdgConfigHome;
    }

    using T = std::optional<std::string>;
    std::pair<T, T> findConfig(std::string programName) {
        std::string           configPath;
        std::set<std::string> paths;

        static const auto     xdgConfigHome = getXdgConfigHome();
        if (xdgConfigHome.has_value())
            paths.insert(xdgConfigHome.value());

        static const auto home = getHome();
        if (home.has_value())
            paths.insert(home.value());

        static const auto xdgConfigDirs = getXdgConfigDirs();
        if (xdgConfigDirs.has_value()) {
            for (auto dir : xdgConfigDirs.value())
                paths.insert(dir);
        }

        paths.insert("/etc/xdg");

        for (auto path : paths) {
            configPath = fullConfigPath(path, programName);
            if (std::filesystem::exists(configPath))
                return std::make_pair(configPath, path);
            else
                Debug::log(LOG, "No config found {}", configPath);
        }

        if (xdgConfigHome.has_value())
            return std::make_pair(std::nullopt, xdgConfigHome);
        else if (home.has_value())
            return std::make_pair(std::nullopt, home);

        Debug::log(ERR, "No config file could be found. Check previous logs for clues");
        return std::make_pair(std::nullopt, std::nullopt);
    }
}
