#pragma once

#include <hyprutils/state/KVStateFile.hpp>

#include <expected>
#include <functional>
#include <system_error>

namespace Hyprutils::State {
    class IKVStateFileBackend {
      public:
        virtual ~IKVStateFileBackend() = default;

        // nullopt means the file does not exist, not a read error.
        virtual std::expected<std::optional<std::string>, std::error_code> read() = 0;
        // Failure must leave the previous contents intact.
        virtual std::expected<void, std::error_code> write(std::string_view contents) = 0;
    };

    using KVStateClock = std::function<CKVStateEntry::TimePoint()>;

    Memory::CUniquePointer<IKVStateFileBackend> makeKVStateFileBackend(std::filesystem::path path);

    struct SKVStateFileAccess {
        static Memory::CUniquePointer<CKVStateFile> create(Memory::CUniquePointer<IKVStateFileBackend> backend, KVStateClock clock);
    };
}
