#include <hyprutils/string/VarList.hpp>

#include <gtest/gtest.h>

using namespace Hyprutils::String;

TEST(String, varlist) {
    CVarList list("hello    world!", 0, 's', true);
    EXPECT_EQ(list[0], "hello");
    EXPECT_EQ(list[1], "world!");
}