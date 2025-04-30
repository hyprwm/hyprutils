#include <csignal>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <hyprutils/os/Process.hpp>
#include "shared.hpp"

using namespace Hyprutils::OS;

int main(int argc, char** argv, char** envp) {
    int      ret = 0;

    CProcess process("sh", {"-c", "echo \"Hello $WORLD!\""});
    process.addEnv("WORLD", "World");

    EXPECT(process.runAsync(), true);
    EXPECT(process.runSync(), true);

    EXPECT(process.stdOut(), std::string{"Hello World!\n"});
    EXPECT(process.stdErr(), std::string{""});
    EXPECT(process.exitCode(), 0);

    CProcess process2("sh", {"-c", "while true; do sleep 1; done;"});

    EXPECT(process2.runAsync(), true);
    EXPECT(getpgid(process2.pid()) >= 0, true);

    kill(process2.pid(), SIGKILL);

    CProcess process3("sh", {"-c", "cat /geryueruggbuergheruger/reugiheruygyuerghuryeghyer/eruihgyuerguyerghyuerghuyergerguyer/NON_EXISTENT"});
    EXPECT(process3.runSync(), true);
    EXPECT(process3.exitCode(), 1);

    return ret;
}