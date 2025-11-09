#include <hyprutils/string/String.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/string/VarList2.hpp>
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

    CConstVarList listConst("hello    world!", 0, 's', true);
    EXPECT(listConst[0], "hello");
    EXPECT(listConst[1], "world!");

    CConstVarList listConst2("0 set", 2, ' ');
    EXPECT(listConst2[0], "0");
    EXPECT(listConst2[1], "set");

    CVarList2 varList2("0 set", 2, ' ');
    EXPECT(varList2[0], "0");
    EXPECT(varList2[1], "set");

    varList2.append("Hello");

    EXPECT(varList2[1], "set");
    EXPECT(varList2[2], "Hello");

    CVarList2 varList2B("hello, world\\, ok?", 0, ',', true, true);
    EXPECT(varList2B[0], "hello");
    EXPECT(varList2B[1], "world, ok?");

    CVarList2 varList2C("hello, , ok?", 0, ',', true, true);
    EXPECT(varList2C[0], "hello");
    EXPECT(varList2C[1], "ok?");

    CVarList2 varList2D("\\\\, , ok?", 0, ',', true, true);
    EXPECT(varList2D[0], "\\");
    EXPECT(varList2D[1], "ok?");

    CVarList2 varList2E("\\, , ok?", 0, ',', true, true);
    EXPECT(varList2E[0], ",");
    EXPECT(varList2E[1], "ok?");

    CVarList2 varList2F("Hello, world\\\\, ok?", 0, ',', true, true);
    EXPECT(varList2F[0], "Hello");
    EXPECT(varList2F[1], "world\\");
    EXPECT(varList2F[2], "ok?");

    CVarList2 varList2G("Hello,\\, ok?", 0, ',', true, true);
    EXPECT(varList2G[0], "Hello");
    EXPECT(varList2G[1], ", ok?");

    CVarList2 varList2H("Hello,\\\\, ok?", 0, ',', true, true);
    EXPECT(varList2H[0], "Hello");
    EXPECT(varList2H[1], "\\");
    EXPECT(varList2H[2], "ok?");

    std::string hello = "hello world!";
    replaceInString(hello, "hello", "hi");
    EXPECT(hello, "hi world!");

    return ret;
}