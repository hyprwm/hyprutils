#include <gtest/gtest.h>
#include <i18n/I18nEngine.hpp>

using namespace Hyprutils::I18n;

TEST(I18n, Locale) {
    EXPECT_EQ(extractLocale("pl_PL.UTF-8"), "pl_PL");
    EXPECT_EQ(extractLocale("POSIX"), "en_US");
    EXPECT_EQ(extractLocale("*"), "en_US");
    EXPECT_EQ(extractLocale("LC_CTYPE=pl_PL.UTF-8"), "pl_PL");
}