#include "KVStateFile.hpp"

#include <hyprutils/memory/Casts.hpp>
#include <hyprutils/utils/ScopeGuard.hpp>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <random>

using namespace Hyprutils::State;
using namespace Hyprutils::Memory;
using namespace Hyprutils::Utils;

static std::unexpected<std::error_code> ioError() {
    return std::unexpected(std::make_error_code(std::errc::io_error));
}

static std::expected<void, std::error_code> writeAll(std::ofstream& stream, std::string_view contents) {
    while (!contents.empty()) {
        const auto count = std::min(contents.size(), size_t{64 * 1024});
        stream.write(contents.data(), sc<std::streamsize>(count));
        if (!stream)
            return ioError();
        contents.remove_prefix(count);
    }
    return {};
}

namespace {
    class CKVStateFileBackend final : public IKVStateFileBackend {
      public:
        explicit CKVStateFileBackend(std::filesystem::path path) : m_path(std::filesystem::absolute(path, m_pathError)) {}

        std::expected<std::optional<std::string>, std::error_code> read() override {
            if (m_pathError)
                return std::unexpected(m_pathError);

            std::error_code error;
            const auto      status = std::filesystem::status(m_path, error);
            if (error && error != std::errc::no_such_file_or_directory)
                return std::unexpected(error);
            if (!std::filesystem::exists(status))
                return std::nullopt;
            if (!std::filesystem::is_regular_file(status))
                return std::unexpected(std::make_error_code(std::filesystem::is_directory(status) ? std::errc::is_a_directory : std::errc::operation_not_supported));

            std::ifstream stream(m_path, std::ios::binary);
            if (!stream)
                return ioError();

            std::string contents;
            char        buffer[16 * 1024];
            while (stream.read(buffer, sizeof(buffer)))
                contents.append(buffer, sizeof(buffer));
            contents.append(buffer, sc<size_t>(stream.gcount()));
            if (!stream.eof() || stream.bad())
                return ioError();
            return std::optional{std::move(contents)};
        }

        std::expected<void, std::error_code> write(std::string_view contents) override {
            if (m_pathError)
                return std::unexpected(m_pathError);

            std::error_code error;
            const auto      parentStatus = std::filesystem::status(m_path.parent_path(), error);
            if (error)
                return std::unexpected(error);
            if (!std::filesystem::exists(parentStatus))
                return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
            if (!std::filesystem::is_directory(parentStatus))
                return std::unexpected(std::make_error_code(std::errc::not_a_directory));

            std::random_device random;
            const auto         directory = m_path.parent_path() / std::format(".hyprutils-kvstate-{:08x}{:08x}", random(), random());
            const auto         temporary = directory / "state";
            bool               pending   = false;
            CScopeGuard        cleanup([&] {
                if (pending) {
                    std::error_code ignored;
                    std::filesystem::remove_all(directory, ignored);
                }
            });

            if (!std::filesystem::create_directory(directory, error))
                return std::unexpected(error ? error : std::make_error_code(std::errc::file_exists));
            pending = true;

            // Prevent readers from opening the file before its permissions have been restricted.
            std::filesystem::permissions(directory, std::filesystem::perms::owner_all, std::filesystem::perm_options::replace, error);
            if (error)
                return std::unexpected(error);

            std::ofstream stream(temporary, std::ios::binary | std::ios::noreplace);
            if (!stream)
                return ioError();

            // Restrict the new file before writing any contents.
            std::filesystem::permissions(temporary, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, std::filesystem::perm_options::replace, error);
            if (error)
                return std::unexpected(error);
            if (const auto result = writeAll(stream, contents); !result)
                return result;

            stream.close();
            if (!stream)
                return ioError();
            std::filesystem::rename(temporary, m_path, error);
            if (error)
                return std::unexpected(error);

            return {};
        }

      private:
        std::error_code       m_pathError;
        std::filesystem::path m_path;
    };
}

CUniquePointer<IKVStateFileBackend> Hyprutils::State::makeKVStateFileBackend(std::filesystem::path path) {
    return makeUnique<CKVStateFileBackend>(std::move(path));
}
