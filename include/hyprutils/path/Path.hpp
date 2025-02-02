#pragma once
#include "../string/VarList.hpp"
#include <string>
#include <optional>
#include <utility>

//NOLINTNEXTLINE
namespace Hyprutils::Path {
    /** Check whether a config in the form basePath/hypr/programName.conf exists.
            @param basePath the path where the config will be searched
            @param programName name of the program (and config file) to search for
        */
    bool checkConfigExists(const std::string basePath, const std::string programName);

    /** Constructs a full config path given the basePath and programName.
            @param basePath the path where the config hypr/programName.conf is located
            @param programName name of the program (and config file)
        */
    std::string fullConfigPath(const std::string basePath, const std::string programName);

    /** Retrieves the absolute path of the $HOME env variable.
        */
    std::optional<std::string> getHome();

    /** Retrieves a CVarList of paths from the $XDG_CONFIG_DIRS env variable.
        */
    std::optional<String::CVarList> getXdgConfigDirs();

    /** Retrieves the absolute path of the $XDG_CONFIG_HOME env variable.
        */
    std::optional<std::string> getXdgConfigHome();

    /** Searches for a config according to the XDG Base Directory specification.
            Returns a pair of the full path to a config and the base path.
            Returns std::nullopt in case of a non-existent value.
            @param programName name of the program (and config file)
        */

    using T = std::optional<std::string>;
    std::pair<T, T> findConfig(const std::string programName);
}
