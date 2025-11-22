#include <hyprutils/string/String.hpp>

#include <gtest/gtest.h>

using namespace Hyprutils::String;

TEST(String, string) {
    EXPECT_EQ(trim("               a             "), "a");
    EXPECT_EQ(trim("   a   a           "), "a   a");
    EXPECT_EQ(trim("a"), "a");
    EXPECT_EQ(trim("                           "), "");

    EXPECT_EQ(isNumber("99214123434"), true);
    EXPECT_EQ(isNumber("-35252345234"), true);
    EXPECT_EQ(isNumber("---3423--432"), false);
    EXPECT_EQ(isNumber("s---3423--432"), false);
    EXPECT_EQ(isNumber("---3423--432s"), false);
    EXPECT_EQ(isNumber("1s"), false);
    EXPECT_EQ(isNumber(""), false);
    EXPECT_EQ(isNumber("-"), false);
    EXPECT_EQ(isNumber("--0"), false);
    EXPECT_EQ(isNumber("abc"), false);
    EXPECT_EQ(isNumber("0.0", true), true);
    EXPECT_EQ(isNumber("0.2", true), true);
    EXPECT_EQ(isNumber("0.", true), false);
    EXPECT_EQ(isNumber(".0", true), false);
    EXPECT_EQ(isNumber("", true), false);
    EXPECT_EQ(isNumber("vvss", true), false);
    EXPECT_EQ(isNumber("0.9999s", true), false);
    EXPECT_EQ(isNumber("s0.9999", true), false);
    EXPECT_EQ(isNumber("-1.0", true), true);
    EXPECT_EQ(isNumber("-1..0", true), false);
    EXPECT_EQ(isNumber("-10.0000000001", true), true);

    EXPECT_EQ(truthy("frgeujgeruibger"), false);
    EXPECT_EQ(truthy("false"), false);
    EXPECT_EQ(truthy("0"), false);
    EXPECT_EQ(truthy("yees"), false);
    EXPECT_EQ(truthy("naa"), false);
    EXPECT_EQ(truthy("-1"), false);
    EXPECT_EQ(truthy("true"), true);
    EXPECT_EQ(truthy("true eeee ee"), true);
    EXPECT_EQ(truthy("yesss"), true);
    EXPECT_EQ(truthy("1"), true);
    EXPECT_EQ(truthy("on"), true);
    EXPECT_EQ(truthy("onn"), true);
}