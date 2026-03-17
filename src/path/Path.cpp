#include <hyprutils/path/Path.hpp>
#include <hyprutils/string/VarList.hpp>
#include <filesystem>
#include <expected>

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

    std::expected<std::string, std::string> resolvePath(const std::string& path, const std::string& base) {
        if (path.empty())
            return std::unexpected("empty path");

        std::filesystem::path p(path);

        // Expand leading ~ to the home directory
        if (path[0] == '~') {
            const auto homeDir = getenv("HOME");
            if (!homeDir)
                return std::unexpected("HOME environment variable not set");

            std::string expanded(homeDir);
            if (path.size() > 1) {
                if (path[1] != '/')
                    return std::unexpected("invalid path: ~ must be followed by / or end of path");
                expanded.append(path.substr(1));
            }
            p = std::filesystem::path(expanded);
        }

        // If still relative, anchor to base (or cwd)
        if (p.is_relative()) {
            std::filesystem::path baseP;
            if (!base.empty()) {
                baseP = std::filesystem::path(base);
                if (baseP.is_relative()) {
                    std::error_code ec;
                    auto            cwd = std::filesystem::current_path(ec);
                    if (ec)
                        return std::unexpected("failed to get current working directory: " + ec.message());
                    baseP = cwd / baseP;
                }
            } else {
                std::error_code ec;
                baseP = std::filesystem::current_path(ec);
                if (ec)
                    return std::unexpected("failed to get current working directory: " + ec.message());
            }
            p = baseP / p;
        }

        // Lexically normalise (resolves . and .. without hitting the filesystem)
        p = p.lexically_normal();

        return p.string();
    }
}
