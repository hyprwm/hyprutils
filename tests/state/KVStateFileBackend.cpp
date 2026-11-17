#include "state/KVStateFile.hpp"

#include <hyprutils/memory/Casts.hpp>
#include <hyprutils/utils/ScopeGuard.hpp>
#include <gtest/gtest.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace Hyprutils::State;
using namespace Hyprutils::Memory;
using namespace Hyprutils::Utils;
namespace fs = std::filesystem;

namespace {
    std::set<fs::path> directoryEntries(const fs::path& directory) {
        std::set<fs::path> entries;
        for (const auto& entry : fs::directory_iterator(directory))
            entries.insert(entry.path().filename());
        return entries;
    }

    class KVStateFileBackendTest : public testing::Test {
      public:
        ~KVStateFileBackendTest() override {
            if (!m_directory.empty()) {
                std::error_code error;
                fs::remove_all(m_directory, error);
            }
        }

      protected:
        void SetUp() override {
            auto pattern = (fs::temp_directory_path() / "hyprutils-kvstate-test-XXXXXX").string();
            ASSERT_NE(::mkdtemp(pattern.data()), nullptr) << std::error_code(errno, std::generic_category()).message();
            m_directory = pattern;
        }

        fs::path m_directory;
    };

    class KVStateSyncFailureTest : public KVStateFileBackendTest, public testing::WithParamInterface<bool> {};
}

TEST_F(KVStateFileBackendTest, MissingReadDoesNotCreateFiles) {
    const auto path    = m_directory / "state";
    auto       backend = makeKVStateFileBackend(path);
    const auto result  = backend->read();
    ASSERT_TRUE(result);
    EXPECT_FALSE(result->has_value());

    {
        CKVStateFile state{path};
        EXPECT_TRUE(state.good());
        EXPECT_EQ(state.size(), 0U);
    }
    EXPECT_FALSE(fs::exists(path));
    EXPECT_TRUE(fs::is_empty(m_directory));
}

TEST_F(KVStateFileBackendTest, MissingParentIsNotCreated) {
    auto       backend = makeKVStateFileBackend(m_directory / "missing" / "state");
    const auto loaded  = backend->read();
    ASSERT_TRUE(loaded);
    EXPECT_FALSE(loaded->has_value());

    const auto saved = backend->write("contents");
    ASSERT_FALSE(saved);
    EXPECT_EQ(saved.error(), std::errc::no_such_file_or_directory);
    EXPECT_TRUE(fs::is_empty(m_directory));
}

TEST_F(KVStateFileBackendTest, EmptyFileIsNotMissing) {
    const auto path    = m_directory / "state";
    auto       backend = makeKVStateFileBackend(path);
    ASSERT_TRUE(backend->write(""));
    EXPECT_EQ(fs::file_size(path), 0U);

    const auto result = backend->read();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->has_value());
    EXPECT_TRUE(result->value().empty());
}

TEST_F(KVStateFileBackendTest, CoreRoundTrip) {
    const auto               path = m_directory / "state";
    CKVStateEntry::TimePoint modified;
    {
        CKVStateFile state{path};
        ASSERT_TRUE(state.good());
        ASSERT_TRUE(state.insert("window", {{"workspace", "2"}, {"title", "initial"}}));
        auto* entry = state.get("window");
        ASSERT_NE(entry, nullptr);
        ASSERT_TRUE(entry->set("title", "updated\nwith spaces"));
        modified = entry->lastModified();
        EXPECT_TRUE(state.good());
    }

    CKVStateFile loaded{path};
    ASSERT_TRUE(loaded.good());
    ASSERT_EQ(loaded.size(), 1U);
    const auto* entry = loaded.get("window");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->get("workspace"), std::optional<std::string_view>{"2"});
    EXPECT_EQ(entry->get("title"), std::optional<std::string_view>{"updated\nwith spaces"});
    EXPECT_EQ(entry->lastModified(), modified);
    EXPECT_EQ(directoryEntries(m_directory), (std::set<fs::path>{"state"}));
}

TEST_F(KVStateFileBackendTest, ReplacementIsAtomicAndPrivate) {
    const auto path    = m_directory / "state";
    auto       backend = makeKVStateFileBackend(path);
    ASSERT_TRUE(backend->write("original"));

    EXPECT_EQ(fs::status(path).permissions() & fs::perms::mask, fs::perms::owner_read | fs::perms::owner_write);
    fs::permissions(path, fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read | fs::perms::others_read);
    std::ifstream original(path, std::ios::binary);
    ASSERT_TRUE(original.is_open());

    ASSERT_TRUE(backend->write("replacement"));
    EXPECT_EQ(fs::status(path).permissions() & fs::perms::mask, fs::perms::owner_read | fs::perms::owner_write);
    EXPECT_EQ(std::string(std::istreambuf_iterator<char>{original}, std::istreambuf_iterator<char>{}), "original");
    EXPECT_FALSE(original.bad());

    const auto result = backend->read();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, std::optional<std::string>{"replacement"});
    EXPECT_EQ(directoryEntries(m_directory), (std::set<fs::path>{"state"}));
}

TEST_F(KVStateFileBackendTest, SynchronizesFlushedFileBeforeRenameAndParentAfterwards) {
    const auto path   = m_directory / "state";
    auto       reader = makeKVStateFileBackend(path);
    ASSERT_TRUE(reader->write("original"));
    struct stat parentStatus{};
    ASSERT_EQ(::stat(m_directory.c_str(), &parentStatus), 0);

    int  syncs   = 0;
    auto backend = makeKVStateFileBackend(path, [&](int fd) {
        ++syncs;
        struct stat status{};
        EXPECT_EQ(::fstat(fd, &status), 0);
        const auto visible = reader->read();
        EXPECT_TRUE(visible);
        if (syncs == 1) {
            EXPECT_TRUE(S_ISREG(status.st_mode));
            EXPECT_EQ(status.st_mode & 0777, 0600);
            if (visible) {
                EXPECT_EQ(*visible, std::optional<std::string>{"original"});
            }

            size_t temporaries = 0;
            for (const auto& entry : fs::directory_iterator(m_directory)) {
                if (!entry.is_directory())
                    continue;
                ++temporaries;
                EXPECT_EQ(entry.status().permissions() & fs::perms::mask, fs::perms::owner_all);
                const auto  temporary = entry.path() / "state";
                struct stat temporaryStatus{};
                EXPECT_EQ(::stat(temporary.c_str(), &temporaryStatus), 0);
                EXPECT_EQ(status.st_dev, temporaryStatus.st_dev);
                EXPECT_EQ(status.st_ino, temporaryStatus.st_ino);
                std::ifstream contents(temporary, std::ios::binary);
                EXPECT_EQ(std::string(std::istreambuf_iterator<char>{contents}, std::istreambuf_iterator<char>{}), "replacement");
                EXPECT_FALSE(contents.bad());
            }
            EXPECT_EQ(temporaries, 1U);
        } else {
            EXPECT_EQ(syncs, 2);
            EXPECT_TRUE(S_ISDIR(status.st_mode));
            EXPECT_EQ(status.st_dev, parentStatus.st_dev);
            EXPECT_EQ(status.st_ino, parentStatus.st_ino);
            if (visible) {
                EXPECT_EQ(*visible, std::optional<std::string>{"replacement"});
            }
        }
        return ::fsync(fd);
    });

    ASSERT_TRUE(backend->write("replacement"));
    EXPECT_EQ(syncs, 2);
    EXPECT_EQ(directoryEntries(m_directory), (std::set<fs::path>{"state"}));
}

TEST_F(KVStateFileBackendTest, InterruptedFileAndDirectorySyncAreRetried) {
    int  syncs   = 0;
    auto backend = makeKVStateFileBackend(m_directory / "state", [&](int fd) {
        if (++syncs % 2) {
            errno = EINTR;
            return -1;
        }
        return ::fsync(fd);
    });

    ASSERT_TRUE(backend->write("contents"));
    EXPECT_EQ(syncs, 4);
    const auto result = backend->read();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, std::optional<std::string>{"contents"});
}

TEST_P(KVStateSyncFailureTest, ReportsSyncErrorAndCleansTemporaryForCreationAndReplacement) {
    const bool failDirectory = GetParam();
    const auto path          = m_directory / "state";
    auto       reader        = makeKVStateFileBackend(path);
    for (const bool existing : {false, true}) {
        SCOPED_TRACE(existing);
        if (existing) {
            ASSERT_TRUE(reader->write("original"));
        }

        int        syncs   = 0;
        auto       backend = makeKVStateFileBackend(path, [&](int fd) {
            ++syncs;
            struct stat status{};
            EXPECT_EQ(::fstat(fd, &status), 0);
            if (sc<bool>(S_ISDIR(status.st_mode)) == failDirectory) {
                errno = ENOSPC;
                return -1;
            }
            return ::fsync(fd);
        });

        const auto result = backend->write("replacement");
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error(), std::error_code(ENOSPC, std::generic_category()));
        EXPECT_EQ(syncs, failDirectory ? 2 : 1);
        const auto visible = reader->read();
        ASSERT_TRUE(visible);
        if (failDirectory)
            EXPECT_EQ(*visible, std::optional<std::string>{"replacement"});
        else if (existing)
            EXPECT_EQ(*visible, std::optional<std::string>{"original"});
        else
            EXPECT_FALSE(visible->has_value());
        EXPECT_EQ(directoryEntries(m_directory), (existing || failDirectory ? std::set<fs::path>{"state"} : std::set<fs::path>{}));
    }
}

TEST_P(KVStateSyncFailureTest, FailedSavePreservesMemoryAndRejectsLaterMutations) {
    const bool failDirectory = GetParam();
    const auto path          = m_directory / "state";
    {
        CKVStateFile original{path};
        ASSERT_TRUE(original.insert("entry", {{"value", "original"}}));
    }

    int  syncs   = 0;
    auto backend = makeKVStateFileBackend(path, [&](int fd) {
        ++syncs;
        struct stat status{};
        EXPECT_EQ(::fstat(fd, &status), 0);
        if (sc<bool>(S_ISDIR(status.st_mode)) == failDirectory) {
            errno = EIO;
            return -1;
        }
        return ::fsync(fd);
    });
    auto state   = SKVStateFileAccess::create(std::move(backend), [] { return CKVStateEntry::TimePoint{}; });
    ASSERT_TRUE(state->good());
    auto* entry = state->get("entry");
    ASSERT_NE(entry, nullptr);
    const auto modified = entry->lastModified();

    EXPECT_FALSE(entry->set("value", "replacement"));
    EXPECT_FALSE(state->good());
    EXPECT_EQ(state->size(), 1U);
    EXPECT_EQ(state->get("entry"), entry);
    EXPECT_EQ(entry->get("value"), std::optional<std::string_view>{"original"});
    EXPECT_EQ(entry->lastModified(), modified);
    EXPECT_FALSE(entry->set("value", "later"));
    EXPECT_FALSE(state->insert("later", {}));
    EXPECT_FALSE(state->remove("entry"));
    EXPECT_EQ(state->pruneOld(0), 0U);
    EXPECT_EQ(syncs, failDirectory ? 2 : 1);

    CKVStateFile loaded{path};
    ASSERT_TRUE(loaded.good());
    const auto* persisted = loaded.get("entry");
    ASSERT_NE(persisted, nullptr);
    EXPECT_EQ(persisted->get("value"), std::optional<std::string_view>{failDirectory ? "replacement" : "original"});
    EXPECT_EQ(persisted->lastModified(), failDirectory ? modified + std::chrono::milliseconds{1} : modified);
    EXPECT_EQ(directoryEntries(m_directory), (std::set<fs::path>{"state"}));
}

INSTANTIATE_TEST_SUITE_P(KVState, KVStateSyncFailureTest, testing::Bool(), [](const testing::TestParamInfo<bool>& info) { return info.param ? "Directory" : "File"; });

TEST_F(KVStateFileBackendTest, BinaryPayloadSpansReadAndWriteBuffers) {
    std::string contents(256 * 1024, '\0');
    for (size_t i = 0; i < contents.size(); ++i)
        contents[i] = sc<char>(i % 256);

    auto backend = makeKVStateFileBackend(m_directory / "state");
    ASSERT_TRUE(backend->write(contents));
    const auto result = backend->read();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(result->value(), contents);
}

TEST_F(KVStateFileBackendTest, RelativeFilename) {
    const auto  previous = fs::current_path();
    CScopeGuard restore([&] {
        std::error_code error;
        fs::current_path(previous, error);
    });
    const auto  directoryA = m_directory / "a";
    const auto  directoryB = m_directory / "b";
    ASSERT_TRUE(fs::create_directory(directoryA));
    ASSERT_TRUE(fs::create_directory(directoryB));
    {
        CKVStateFile existing{directoryB / "state"};
        ASSERT_TRUE(existing.good());
        ASSERT_TRUE(existing.insert("sentinel", {{"value", "untouched"}}));
    }
    auto       other    = makeKVStateFileBackend(directoryB / "state");
    const auto original = other->read();
    ASSERT_TRUE(original);
    ASSERT_TRUE(original->has_value());

    fs::current_path(directoryA);
    auto       backend = makeKVStateFileBackend("state");
    const auto missing = backend->read();
    ASSERT_TRUE(missing);
    EXPECT_FALSE(missing->has_value());
    CKVStateFile state{"state"};
    ASSERT_TRUE(state.good());

    fs::current_path(directoryB);
    ASSERT_TRUE(state.insert("relative", {{"value", "saved in A"}}));
    CKVStateFile loaded{directoryA / "state"};
    ASSERT_TRUE(loaded.good());
    ASSERT_EQ(loaded.size(), 1U);
    const auto* entry = loaded.get("relative");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->get("value"), std::optional<std::string_view>{"saved in A"});

    const auto saved  = makeKVStateFileBackend(directoryA / "state")->read();
    const auto result = backend->read();
    ASSERT_TRUE(saved);
    ASSERT_TRUE(saved->has_value());
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, *saved);
    const auto unchanged = other->read();
    ASSERT_TRUE(unchanged);
    EXPECT_EQ(*unchanged, *original);
    EXPECT_EQ(directoryEntries(directoryA), (std::set<fs::path>{"state"}));
    EXPECT_EQ(directoryEntries(directoryB), (std::set<fs::path>{"state"}));
}

TEST_F(KVStateFileBackendTest, DirectoryReadFails) {
    auto       backend = makeKVStateFileBackend(m_directory);
    const auto result  = backend->read();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), std::errc::is_a_directory);

    CKVStateFile state{m_directory};
    EXPECT_FALSE(state.good());
    EXPECT_EQ(state.size(), 0U);
    EXPECT_TRUE(fs::is_empty(m_directory));
}

TEST_F(KVStateFileBackendTest, FifoReadFailsWithoutBlocking) {
    const auto path = m_directory / "fifo";
    ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
    // Bound a blocking-open regression to the child instead of hanging the test runner.
    const auto child = ::fork();
    ASSERT_NE(child, -1);
    if (child == 0) {
        ::alarm(5);
        const auto result = makeKVStateFileBackend(path)->read();
        ::_exit(!result && result.error() == std::errc::operation_not_supported ? 0 : 1);
    }

    int   status = 0;
    pid_t waited;
    do {
        waited = ::waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    ASSERT_EQ(waited, child);
    ASSERT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);
}

TEST_F(KVStateFileBackendTest, NonDirectoryParentFailsAndPreservesData) {
    auto original = makeKVStateFileBackend(m_directory / "state");
    ASSERT_TRUE(original->write("original"));
    auto       backend = makeKVStateFileBackend(m_directory / "state" / "child");
    const auto loaded  = backend->read();
    ASSERT_FALSE(loaded);
    EXPECT_EQ(loaded.error(), std::errc::not_a_directory);

    const auto saved = backend->write("replacement");
    ASSERT_FALSE(saved);
    EXPECT_EQ(saved.error(), std::errc::not_a_directory);
    const auto unchanged = original->read();
    ASSERT_TRUE(unchanged);
    EXPECT_EQ(*unchanged, std::optional<std::string>{"original"});
    EXPECT_EQ(directoryEntries(m_directory), (std::set<fs::path>{"state"}));
}

TEST_F(KVStateFileBackendTest, FailedRenameCleansTemporaryAndPreservesData) {
    const auto destination = m_directory / "destination";
    ASSERT_TRUE(fs::create_directory(destination));
    auto original = makeKVStateFileBackend(destination / "state");
    ASSERT_TRUE(original->write("original"));

    auto       backend = makeKVStateFileBackend(destination);
    const auto result  = backend->write("replacement");
    ASSERT_FALSE(result);
    EXPECT_TRUE(fs::is_directory(destination));
    EXPECT_EQ(directoryEntries(m_directory), (std::set<fs::path>{"destination"}));
    EXPECT_EQ(directoryEntries(destination), (std::set<fs::path>{"state"}));
    const auto unchanged = original->read();
    ASSERT_TRUE(unchanged);
    EXPECT_EQ(*unchanged, std::optional<std::string>{"original"});
}

TEST_F(KVStateFileBackendTest, PartialWriteFailureCleansTemporaryAndPreservesData) {
    const auto path = m_directory / "state";
    {
        CKVStateFile state{path};
        ASSERT_TRUE(state.good());
        ASSERT_TRUE(state.insert("original", {{"value", "preserved"}}));
    }
    auto       backend  = makeKVStateFileBackend(path);
    const auto original = backend->read();
    ASSERT_TRUE(original);
    ASSERT_TRUE(original->has_value());

    const auto child = ::fork();
    ASSERT_NE(child, -1);
    if (child == 0) {
        ::alarm(5);
        if (std::signal(SIGXFSZ, SIG_IGN) == SIG_ERR)
            ::_exit(1);
        // Allow a prefix to be written before the stream reports a write failure.
        const rlimit limit{1024, 1024};
        if (::setrlimit(RLIMIT_FSIZE, &limit) < 0)
            ::_exit(2);
        const auto result = backend->write(std::string(256 * 1024, 'x'));
        ::_exit(!result && result.error() == std::errc::io_error ? 0 : 3);
    }

    int   status = 0;
    pid_t waited;
    do {
        waited = ::waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    ASSERT_EQ(waited, child);
    ASSERT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0) << "1: signal setup failed; 2: limit setup failed; 3: expected stream io_error";

    const auto unchanged = backend->read();
    ASSERT_TRUE(unchanged);
    EXPECT_EQ(*unchanged, *original);
    EXPECT_EQ(directoryEntries(m_directory), (std::set<fs::path>{"state"}));
}
