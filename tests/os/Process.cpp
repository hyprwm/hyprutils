#include <hyprutils/os/Process.hpp>

#include <gtest/gtest.h>

using namespace Hyprutils::OS;

TEST(OS, process) {
    CProcess process("sh", {"-c", "echo \"Hello $WORLD!\""});
    process.addEnv("WORLD", "World");

    EXPECT_EQ(process.runAsync(), true);
    EXPECT_EQ(process.runSync(), true);

    EXPECT_EQ(process.stdOut(), std::string{"Hello World!\n"});
    EXPECT_EQ(process.stdErr(), std::string{""});
    EXPECT_EQ(process.exitCode(), 0);

    CProcess process2("sh", {"-c", "while true; do sleep 1; done;"});

    EXPECT_EQ(process2.runAsync(), true);
    EXPECT_EQ(getpgid(process2.pid()) >= 0, true);

    kill(process2.pid(), SIGKILL);

    CProcess process3("sh", {"-c", "cat /geryueruggbuergheruger/reugiheruygyuerghuryeghyer/eruihgyuerguyerghyuerghuyergerguyer/NON_EXISTENT"});
    EXPECT_EQ(process3.runSync(), true);
    EXPECT_EQ(process3.exitCode(), 1);
}