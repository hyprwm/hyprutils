#include <hyprutils/os/Process.hpp>
using namespace Hyprutils::OS;

#include <csignal>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <array>

#include <sys/wait.h>

#ifdef __APPLE__
#include <signal.h>
#endif

Hyprutils::OS::CProcess::CProcess(const std::string& binary_, const std::vector<std::string>& args_) : binary(binary_), args(args_) {
    ;
}

bool Hyprutils::OS::CProcess::runSync() {
    int outPipe[2], errPipe[2];
    if (pipe(outPipe))
        return false;
    if (pipe(errPipe)) {
        close(outPipe[0]);
        close(outPipe[1]);
        return false;
    }

    int pid = fork();
    if (pid == -1) {
        close(outPipe[0]);
        close(outPipe[1]);
        close(outPipe[0]);
        close(outPipe[1]);
        return false;
    }

    if (!pid) {
        // child
        close(outPipe[0]);
        close(errPipe[0]);

        dup2(outPipe[1], 1 /* stdout */);
        dup2(errPipe[1], 2 /* stderr */);

        // build argv
        std::vector<const char*> argsC;
        argsC.emplace_back(strdup(binary.c_str()));
        for (auto& arg : args) {
            // TODO: does this leak? Can we just pipe c_str() as the strings won't be realloc'd?
            argsC.emplace_back(strdup(arg.c_str()));
        }

        argsC.emplace_back(nullptr);

        execvp(binary.c_str(), (char* const*)argsC.data());
        exit(1);
    } else {
        // parent
        close(outPipe[1]);
        close(errPipe[1]);

        waitpid(pid, nullptr, 0);

        std::string            readOutData;
        std::array<char, 1024> buf;
        buf.fill(0);

        // wait for read
        size_t ret = 0;
        while ((ret = read(outPipe[0], buf.data(), 1023)) > 0) {
            readOutData += std::string{(char*)buf.data(), ret};
        }

        out         = readOutData;
        readOutData = "";

        while ((ret = read(errPipe[0], buf.data(), 1023)) > 0) {
            readOutData += std::string{(char*)buf.data(), ret};
        }

        err = readOutData;

        close(outPipe[0]);
        close(errPipe[0]);

        return true;
    }

    return true;
}

bool Hyprutils::OS::CProcess::runAsync() {
    int socket[2];
    if (pipe(socket) != 0)
        return false;

    pid_t child, grandchild;
    child = fork();
    if (child < 0) {
        close(socket[0]);
        close(socket[1]);
        return false;
    }

    if (child == 0) {
        // run in child
        sigset_t set;
        sigemptyset(&set);
        sigprocmask(SIG_SETMASK, &set, NULL);

        grandchild = fork();
        if (grandchild == 0) {
            // run in grandchild
            close(socket[0]);
            close(socket[1]);
            // build argv
            std::vector<const char*> argsC;
            argsC.emplace_back(strdup(binary.c_str()));
            for (auto& arg : args) {
                // TODO: does this leak? Can we just pipe c_str() as the strings won't be realloc'd?
                argsC.emplace_back(strdup(arg.c_str()));
            }

            argsC.emplace_back(nullptr);

            execvp(binary.c_str(), (char* const*)argsC.data());
            // exit grandchild
            _exit(0);
        }
        close(socket[0]);
        write(socket[1], &grandchild, sizeof(grandchild));
        close(socket[1]);
        // exit child
        _exit(0);
    }
    // run in parent
    close(socket[1]);
    read(socket[0], &grandchild, sizeof(grandchild));
    close(socket[0]);
    // clear child and leave grandchild to init
    waitpid(child, NULL, 0);

    return true;
}

const std::string& Hyprutils::OS::CProcess::stdOut() {
    return out;
}

const std::string& Hyprutils::OS::CProcess::stdErr() {
    return err;
}