#include <hyprutils/string/ConstVarList.hpp>

#include <gtest/gtest.h>

using namespace Hyprutils::String;

TEST(String, constvarlist) {
    CConstVarList listConst("hello    world!", 0, 's', true);
    EXPECT_EQ(listConst[0], "hello");
    EXPECT_EQ(listConst[1], "world!");

    CConstVarList listConst2("0 set", 2, ' ');
    EXPECT_EQ(listConst2[0], "0");
    EXPECT_EQ(listConst2[1], "set");
}