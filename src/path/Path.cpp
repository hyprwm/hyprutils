#include <hyprutils/path/Path.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/debug/Log.hpp>
#include <filesystem>
#include <vector>

using namespace Hyprutils;

namespace Hyprutils::Path {
    std::string fullConfigPath(std::string basePath, std::string programName) {
        return basePath + "/hypr/" + programName + ".conf";
    }

    void push_if_new(std::vector<std::string> vec, std::string str) {
        if (std::find(vec.begin(), vec.end(), str) == vec.end())
            vec.push_back(str);
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
        std::vector<std::string> paths;

        static const auto     xdgConfigHome = getXdgConfigHome();
        if (xdgConfigHome.has_value())
            push_if_new(paths, xdgConfigHome.value());

        static const auto home = getHome();
        if (home.has_value())
            push_if_new(paths, home.value());

        static const auto xdgConfigDirs = getXdgConfigDirs();
        if (xdgConfigDirs.has_value()) {
            for (auto dir : xdgConfigDirs.value())
                push_if_new(paths, dir);
        }

        push_if_new(paths, "/etc/xdg");

        for (auto path : paths) {
            configPath = fullConfigPath(path, programName);
            Debug::log(INFO, "Checking config {}", configPath);
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
