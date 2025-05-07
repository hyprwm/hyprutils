#include <hyprutils/string/String.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/string/ConstVarList.hpp>
#include "shared.hpp"

using namespace Hyprutils::String;

int main(int argc, char** argv, char** envp) {
    int ret = 0;

    EXPECT(trim("               a             "), "a");
    EXPECT(trim("   a   a           "), "a   a");
    EXPECT(trim("a"), "a");
    EXPECT(trim("                           "), "");

    EXPECT(isNumber("99214123434"), true);
    EXPECT(isNumber("-35252345234"), true);
    EXPECT(isNumber("---3423--432"), false);
    EXPECT(isNumber("s---3423--432"), false);
    EXPECT(isNumber("---3423--432s"), false);
    EXPECT(isNumber("1s"), false);
    EXPECT(isNumber(""), false);
    EXPECT(isNumber("-"), false);
    EXPECT(isNumber("--0"), false);
    EXPECT(isNumber("abc"), false);
    EXPECT(isNumber("0.0", true), true);
    EXPECT(isNumber("0.2", true), true);
    EXPECT(isNumber("0.", true), false);
    EXPECT(isNumber(".0", true), false);
    EXPECT(isNumber("", true), false);
    EXPECT(isNumber("vvss", true), false);
    EXPECT(isNumber("0.9999s", true), false);
    EXPECT(isNumber("s0.9999", true), false);
    EXPECT(isNumber("-1.0", true), true);
    EXPECT(isNumber("-1..0", true), false);
    EXPECT(isNumber("-10.0000000001", true), true);

    CVarList list("hello    world!", 0, 's', true);
    EXPECT(list[0], "hello");
    EXPECT(list[1], "world!");

    CVarList list2("test:test\\:test", 0, ':', true, true);
    EXPECT(list2[0], "test");
    EXPECT(list2[1], "test:test");

    CConstVarList listConst("hello    world!", 0, 's', true);
    EXPECT(listConst[0], "hello");
    EXPECT(listConst[1], "world!");

    std::string hello = "hello world!";
    replaceInString(hello, "hello", "hi");
    EXPECT(hello, "hi world!");

    return ret;
}