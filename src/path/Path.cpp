#include <hyprutils/path/Path.hpp>
#include <hyprutils/string/VarList.hpp>
#include <filesystem>

using namespace Hyprutils;

namespace Hyprutils::Path {
    std::string fullConfigPath(std::string basePath, std::string programName) {
        //no lint or...?
        return basePath + "/hypr/" + programName + ".conf";
    }

    bool checkConfigExists(std::string basePath, std::string programName) {
        return std::filesystem::exists(fullConfigPath(basePath, programName));
    }

    std::optional<std::string> getHome() {
        static const auto homeDir = getenv("HOME");

        if (!homeDir || !std::filesystem::path(homeDir).is_absolute())
            return std::nullopt;

        return std::string(homeDir).append("/.config");
    }

    std::optional<String::CVarList> getXdgConfigDirs() {
        static const auto xdgConfigDirs = getenv("XDG_CONFIG_DIRS");

        if (!xdgConfigDirs)
            return std::nullopt;

        static const auto xdgConfigDirsList = String::CVarList(xdgConfigDirs, 0, ':');

        return xdgConfigDirsList;
    }

    std::optional<std::string> getXdgConfigHome() {
        static const auto xdgConfigHome = getenv("XDG_CONFIG_HOME");

        if (!xdgConfigHome || !std::filesystem::path(xdgConfigHome).is_absolute())
            return std::nullopt;

        return xdgConfigHome;
    }

    using T = std::optional<std::string>;
    std::pair<T, T> findConfig(std::string programName) {
        bool              xdgConfigHomeExists = false;
        static const auto xdgConfigHome       = getXdgConfigHome();
        if (xdgConfigHome.has_value()) {
            xdgConfigHomeExists = true;
            if (checkConfigExists(xdgConfigHome.value(), programName))
                return std::make_pair(fullConfigPath(xdgConfigHome.value(), programName), xdgConfigHome);
        }

        bool              homeExists = false;
        static const auto home       = getHome();
        if (home.has_value()) {
            homeExists = true;
            if (checkConfigExists(home.value(), programName))
                return std::make_pair(fullConfigPath(home.value(), programName), home);
        }

        static const auto xdgConfigDirs = getXdgConfigDirs();
        if (xdgConfigDirs.has_value()) {
            for (auto& dir : xdgConfigDirs.value()) {
                if (checkConfigExists(dir, programName))
                    return std::make_pair(fullConfigPath(dir, programName), std::nullopt);
            }
        }

        if (checkConfigExists("/etc/xdg", programName))
            return std::make_pair(fullConfigPath("/etc/xdg", programName), std::nullopt);

        if (xdgConfigHomeExists)
            return std::make_pair(std::nullopt, xdgConfigHome);
        else if (homeExists)
            return std::make_pair(std::nullopt, home);

        return std::make_pair(std::nullopt, std::nullopt);
    }
}
