#include <hyprutils/os/File.hpp>

#include <filesystem>
#include <fstream>

using namespace Hyprutils;
using namespace Hyprutils::File;

std::expected<std::string, std::string> File::readFileAsString(const std::string_view& path) {
    std::error_code ec;

    if (!std::filesystem::exists(path, ec) || ec)
        return std::unexpected("File not found");

    std::ifstream file(std::string{path});
    if (!file.good())
        return std::unexpected("Failed to open file");

    return std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
}
