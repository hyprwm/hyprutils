#include <algorithm>
#include <hyprutils/string/String.hpp>

using namespace Hyprutils::String;

std::string Hyprutils::String::trim(const std::string& in) {
    if (in.empty())
        return in;

    size_t countBefore = 0;
    while (countBefore < in.length() && std::isspace(in.at(countBefore))) {
        countBefore++;
    }

    size_t countAfter = 0;
    while (countAfter < in.length() - countBefore && std::isspace(in.at(in.length() - countAfter - 1))) {
        countAfter++;
    }

    std::string result = in.substr(countBefore, in.length() - countBefore - countAfter);

    return result;
}

std::string_view Hyprutils::String::trim(const std::string_view& sv) {
    if (sv.empty())
        return sv;

    size_t countBefore = 0;
    while (countBefore < sv.length() && std::isspace(sv.at(countBefore))) {
        countBefore++;
    }

    size_t countAfter = 0;
    while (countAfter < sv.length() - countBefore && std::isspace(sv.at(sv.length() - countAfter - 1))) {
        countAfter++;
    }

    return sv.substr(countBefore, sv.length() - countBefore - countAfter);
}

std::string Hyprutils::String::trim(const char* in) {
    return trim(std::string{in});
}

bool Hyprutils::String::isNumber(const std::string& str, bool allowfloat) {
    if (str.empty())
        return false;

    bool decimalParsed = false;

    for (size_t i = 0; i < str.length(); ++i) {
        const char& c = str.at(i);

        if (i == 0 && str.at(i) == '-') {
            // only place where we allow -
            continue;
        }

        if (!isdigit(c)) {
            if (!allowfloat)
                return false;

            if (c != '.')
                return false;

            if (i == 0)
                return false;

            if (decimalParsed)
                return false;

            decimalParsed = true;

            continue;
        }
    }

    return isdigit(str.back()) != 0;
}

void Hyprutils::String::replaceInString(std::string& string, const std::string& what, const std::string& to) {
    if (string.empty())
        return;
    size_t pos = 0;
    while ((pos = string.find(what, pos)) != std::string::npos) {
        string.replace(pos, what.length(), to);
        pos += to.length();
    }
}

#ifdef HU_UNIT_TESTS

#include <gtest/gtest.h>

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
}

#endif