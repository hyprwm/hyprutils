#include <hyprutils/os/ProcLock.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

using namespace Hyprutils::OS;

class ProcLockTest : public testing::Test {
  protected:
    void SetUp() override {
        if (const auto* runtimeDir = std::getenv("XDG_RUNTIME_DIR"))
            m_oldRuntimeDir = runtimeDir;

        m_runtimeDir = std::filesystem::temp_directory_path() / std::format("hyprutils-proclock-test-{}", getpid());
        std::filesystem::remove_all(m_runtimeDir);
        std::filesystem::create_directories(m_runtimeDir);
        ASSERT_EQ(setenv("XDG_RUNTIME_DIR", m_runtimeDir.c_str(), 1), 0);
    }

    void TearDown() override {
        if (m_oldRuntimeDir)
            setenv("XDG_RUNTIME_DIR", m_oldRuntimeDir->c_str(), 1);
        else
            unsetenv("XDG_RUNTIME_DIR");

        std::filesystem::remove_all(m_runtimeDir);
    }

    std::filesystem::path      m_runtimeDir;
    std::optional<std::string> m_oldRuntimeDir;
};

TEST_F(ProcLockTest, ExclusiveProperties) {
    CProcLock first{"instance", {{"display", "one"}}};
    ASSERT_TRUE(first.obtain(CProcLock::eProcLockFlags::EXCLUSIVE));

    CProcLock  matching{"instance", {{"display", "one"}}};
    const auto matchingResult = matching.obtain(CProcLock::eProcLockFlags::EXCLUSIVE);
    ASSERT_FALSE(matchingResult);
    EXPECT_EQ(matchingResult.error(), CProcLock::eProcLockObtainingError::ALREADY_RUNNING);

    CProcLock different{"instance", {{"display", "two"}}};
    EXPECT_TRUE(different.obtain(CProcLock::eProcLockFlags::EXCLUSIVE));
}

TEST_F(ProcLockTest, ObtainOnlyOnce) {
    CProcLock lock{"instance", {}};
    ASSERT_TRUE(lock.obtain(CProcLock::eProcLockFlags::NO_FLAGS));

    const auto secondResult = lock.obtain(CProcLock::eProcLockFlags::NO_FLAGS);
    ASSERT_FALSE(secondResult);
    EXPECT_EQ(secondResult.error(), CProcLock::eProcLockObtainingError::ALREADY_TAKEN);
    EXPECT_EQ(lock.enumerate().size(), 1);
}

TEST_F(ProcLockTest, ChildDoesNotReleaseParentLock) {
    CProcLock parent{"instance", {}};
    ASSERT_TRUE(parent.obtain(CProcLock::eProcLockFlags::EXCLUSIVE));

    const auto child = fork();
    ASSERT_NE(child, -1);

    if (child == 0) {
        parent.~CProcLock();
        _exit(0);
    }

    int status = 0;
    ASSERT_EQ(waitpid(child, &status, 0), child);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);

    CProcLock  contender{"instance", {}};
    const auto contenderResult = contender.obtain(CProcLock::eProcLockFlags::EXCLUSIVE);
    ASSERT_FALSE(contenderResult);
    EXPECT_EQ(contenderResult.error(), CProcLock::eProcLockObtainingError::ALREADY_RUNNING);
}

TEST_F(ProcLockTest, RuntimeDirectoryIsNotCached) {
    CProcLock first{"first", {}};
    ASSERT_TRUE(first.obtain(CProcLock::eProcLockFlags::NO_FLAGS));

    const auto secondRuntimeDir = m_runtimeDir / "second";
    ASSERT_TRUE(std::filesystem::create_directories(secondRuntimeDir));
    ASSERT_EQ(setenv("XDG_RUNTIME_DIR", secondRuntimeDir.c_str(), 1), 0);

    CProcLock second{"second", {}};
    ASSERT_TRUE(second.obtain(CProcLock::eProcLockFlags::NO_FLAGS));
    EXPECT_TRUE(std::filesystem::exists(secondRuntimeDir / ".hyprlocks"));
}
