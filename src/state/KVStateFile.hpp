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
        // Success confirms atomic replacement and file/parent-directory synchronization.
        // Failure before replacement preserves the old contents. Failure afterwards is an uncertain commit, not a rollback.
        virtual std::expected<void, std::error_code> write(std::string_view contents) = 0;
    };

    using KVStateClock    = std::function<CKVStateEntry::TimePoint()>;
    using KVStateFileSync = std::function<int(int)>;

    // The optional sync callback replaces fsync for tests and follows its return/errno convention.
    Memory::CUniquePointer<IKVStateFileBackend> makeKVStateFileBackend(std::filesystem::path path, KVStateFileSync sync = {});

    struct SKVStateFileAccess {
        static Memory::CUniquePointer<CKVStateFile> create(Memory::CUniquePointer<IKVStateFileBackend> backend, KVStateClock clock);
    };
}
