#include <hyprutils/string/VarList2.hpp>

#include <gtest/gtest.h>

using namespace Hyprutils::String;

TEST(String, varlist2) {
    CVarList2 varList2("0 set", 2, ' ');
    EXPECT_EQ(varList2[0], "0");
    EXPECT_EQ(varList2[1], "set");

    varList2.append("Hello");

    EXPECT_EQ(varList2[1], "set");
    EXPECT_EQ(varList2[2], "Hello");
    EXPECT_EQ(varList2[3], "");
    EXPECT_EQ(varList2.contains("set"), true);
    EXPECT_EQ(varList2.contains("sett"), false);
    EXPECT_EQ(varList2.contains(""), false);
    EXPECT_EQ(varList2.size(), 3);

    CVarList2 varList2B("hello, world\\, ok?", 0, ',', true, true);
    EXPECT_EQ(varList2B[0], "hello");
    EXPECT_EQ(varList2B[1], "world, ok?");

    CVarList2 varList2C("hello, , ok?", 0, ',', true, true);
    EXPECT_EQ(varList2C[0], "hello");
    EXPECT_EQ(varList2C[1], "ok?");

    CVarList2 varList2D("\\\\, , ok?", 0, ',', true, true);
    EXPECT_EQ(varList2D[0], "\\");
    EXPECT_EQ(varList2D[1], "ok?");

    CVarList2 varList2E("\\, , ok?", 0, ',', true, true);
    EXPECT_EQ(varList2E[0], ",");
    EXPECT_EQ(varList2E[1], "ok?");

    CVarList2 varList2F("Hello, world\\\\, ok?", 0, ',', true, true);
    EXPECT_EQ(varList2F[0], "Hello");
    EXPECT_EQ(varList2F[1], "world\\");
    EXPECT_EQ(varList2F[2], "ok?");

    CVarList2 varList2G("Hello,\\, ok?", 0, ',', true, true);
    EXPECT_EQ(varList2G[0], "Hello");
    EXPECT_EQ(varList2G[1], ", ok?");

    CVarList2 varList2H("Hello,\\\\, ok?", 0, ',', true, true);
    EXPECT_EQ(varList2H[0], "Hello");
    EXPECT_EQ(varList2H[1], "\\");
    EXPECT_EQ(varList2H[2], "ok?");

    CVarList2 varList2I("Hello,\\, ok?", 0, ',', true, false);
    EXPECT_EQ(varList2I[0], "Hello");
    EXPECT_EQ(varList2I[1], "\\");
    EXPECT_EQ(varList2I[2], "ok?");

    CVarList2 varList2J("", 0, ',', true, false);
    EXPECT_EQ(varList2J[0], "");

    CVarList2 varList2K(",\\, ok?", 0, ',', true);
    EXPECT_EQ(varList2K[0], ", ok?");

    CVarList2 varList2L("Hello, world", 0, ',', true);
    EXPECT_EQ(varList2L.join(" "), "Hello world");
    EXPECT_EQ(varList2L.join(" ", 0, 1000), "Hello world");
    EXPECT_EQ(varList2L.join(" ", 0, 1), "Hello");
}