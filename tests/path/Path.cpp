#include <hyprutils/path/Path.hpp>

#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>

using namespace Hyprutils::Path;

static std::string cwd() {
    return std::filesystem::current_path().string();
}

static std::string home() {
    const char* h = getenv("HOME");
    return h ? std::string(h) : std::string{};
}

TEST(Path, resolvePath_absolute) {
    EXPECT_EQ(resolvePath("/foo/bar/baz").value(), "/foo/bar/baz");
    EXPECT_EQ(resolvePath("/foo/./bar/../baz").value(), "/foo/baz");
    EXPECT_EQ(resolvePath("/a/b/../../c").value(), "/c");
    EXPECT_EQ(resolvePath("/a/b/c/./././../d").value(), "/a/b/d");
}

TEST(Path, resolvePath_dot) {
    const std::string base = "/some/base";
    EXPECT_EQ(resolvePath("./here", base).value(), "/some/base/here");
    EXPECT_EQ(resolvePath("./a/b/c", base).value(), "/some/base/a/b/c");
    EXPECT_EQ(resolvePath("./here").value(), cwd() + "/here");
}

TEST(Path, resolvePath_dotdot) {
    const std::string base = "/some/base/dir";
    EXPECT_EQ(resolvePath("../here", base).value(), "/some/base/here");
    EXPECT_EQ(resolvePath("../../here", base).value(), "/some/here");
    EXPECT_EQ(resolvePath("../../../../././.here", base).value(), "/.here");
    EXPECT_EQ(resolvePath("./a/../../b/../c", base).value(), "/some/base/c");
}

TEST(Path, resolvePath_tilde) {
    const std::string h = home();
    if (h.empty())
        GTEST_SKIP() << "HOME not set, skipping ~ tests";

    EXPECT_EQ(resolvePath("~").value(), h);
    EXPECT_EQ(resolvePath("~/").value(), h + "/");
    EXPECT_EQ(resolvePath("~/foo/bar").value(), h + "/foo/bar");
    EXPECT_EQ(resolvePath("~/foo/../bar").value(), h + "/bar");
    EXPECT_EQ(resolvePath("~/./foo/./bar").value(), h + "/foo/bar");
}

TEST(Path, resolvePath_relative_base) {
    EXPECT_EQ(resolvePath("file.txt", "subdir").value(), cwd() + "/subdir/file.txt");
    EXPECT_EQ(resolvePath("../file.txt", "subdir").value(), cwd() + "/file.txt");
}

TEST(Path, resolvePath_errors) {
    EXPECT_FALSE(resolvePath("").has_value());

    EXPECT_FALSE(resolvePath("~nohome").has_value());
}

class DataDirTest : public testing::Test {
  protected:
    void SetUp() override {
        if (const auto homeDir = std::getenv("HOME"))
            m_oldHome = homeDir;
        if (const auto dataHome = std::getenv("XDG_DATA_HOME"))
            m_oldDataHome = dataHome;

        ASSERT_EQ(setenv("HOME", "/home/test", 1), 0);
        ASSERT_EQ(unsetenv("XDG_DATA_HOME"), 0);
    }

    void TearDown() override {
        if (m_oldHome)
            setenv("HOME", m_oldHome->c_str(), 1);
        else
            unsetenv("HOME");

        if (m_oldDataHome)
            setenv("XDG_DATA_HOME", m_oldDataHome->c_str(), 1);
        else
            unsetenv("XDG_DATA_HOME");
    }

    std::optional<std::string> m_oldHome;
    std::optional<std::string> m_oldDataHome;
};

TEST_F(DataDirTest, UsesXdgDataHome) {
    ASSERT_EQ(setenv("XDG_DATA_HOME", "/custom/data", 1), 0);
    EXPECT_EQ(getDataDir("hyprpaper").value(), "/custom/data/hyprpaper");
    EXPECT_EQ(getDataDir("hyprland").value(), "/custom/data/hyprland");

    ASSERT_EQ(setenv("XDG_DATA_HOME", "/other/data/", 1), 0);
    ASSERT_EQ(unsetenv("HOME"), 0);
    EXPECT_EQ(getDataDir("hyprpaper").value(), "/other/data/hyprpaper");
}

TEST_F(DataDirTest, FallsBackToHome) {
    EXPECT_EQ(getDataDir("hyprpaper").value(), "/home/test/.local/share/hyprpaper");

    for (const auto dataHome : {"", "relative/data"}) {
        ASSERT_EQ(setenv("XDG_DATA_HOME", dataHome, 1), 0);
        EXPECT_EQ(getDataDir("hyprpaper").value(), "/home/test/.local/share/hyprpaper");
    }

    ASSERT_EQ(setenv("HOME", "/home/other/", 1), 0);
    EXPECT_EQ(getDataDir("hyprpaper").value(), "/home/other/.local/share/hyprpaper");
}

TEST_F(DataDirTest, RequiresAbsoluteBase) {
    ASSERT_EQ(unsetenv("HOME"), 0);
    EXPECT_FALSE(getDataDir("hyprpaper").has_value());

    for (const auto homeDir : {"", "relative/home"}) {
        ASSERT_EQ(setenv("HOME", homeDir, 1), 0);
        EXPECT_FALSE(getDataDir("hyprpaper").has_value());
    }
}
