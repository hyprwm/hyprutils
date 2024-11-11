#include <hyprutils/os/Process.hpp>
using namespace Hyprutils::OS;

#include <csignal>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <array>
#include <thread>

#include <sys/fcntl.h>
#include <sys/wait.h>

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

        out = "";
        err = "";

        std::array<char, 1024> buf;
        buf.fill(0);

        // wait for read
        ssize_t ret = 0;

        int     fdFlags = fcntl(outPipe[0], F_GETFL, 0);
        if (fcntl(outPipe[0], F_SETFL, fdFlags | O_NONBLOCK) < 0)
            return false;
        fdFlags = fcntl(errPipe[0], F_GETFL, 0);
        if (fcntl(errPipe[0], F_SETFL, fdFlags | O_NONBLOCK) < 0)
            return false;

        // FIXME: this sucks, but it prevents a pipe deadlock.
        // Problem is, if we exceed the 64k buffer, we end up in a deadlock.
        // So, as a "solution", we keep reading until the child pid exits.
        // If nothing is read from either stdout or stderr, sleep for 100µs, to maybe not peg a core THAT much.
        // If anyone knows a better solution, feel free to make a MR.

        while (waitpid(pid, nullptr, WNOHANG) == 0) {
            int any = 0;

            while ((ret = read(outPipe[0], buf.data(), 1023)) > 0) {
                out += std::string{(char*)buf.data(), (size_t)ret};
            }

            any += errno == EWOULDBLOCK || errno == EAGAIN ? 1 : 0;

            buf.fill(0);

            while ((ret = read(errPipe[0], buf.data(), 1023)) > 0) {
                err += std::string{(char*)buf.data(), (size_t)ret};
            }

            any += errno == EWOULDBLOCK || errno == EAGAIN ? 1 : 0;

            buf.fill(0);

            if (any >= 2)
                std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

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