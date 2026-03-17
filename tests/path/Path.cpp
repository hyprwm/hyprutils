#include <hyprutils/path/Path.hpp>

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
