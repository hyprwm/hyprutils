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
#include <sys/poll.h>

Hyprutils::OS::CProcess::CProcess(const std::string& binary_, const std::vector<std::string>& args_) : binary(binary_), args(args_) {
    ;
}

void Hyprutils::OS::CProcess::addEnv(const std::string& name, const std::string& value) {
    env.emplace_back(std::make_pair<>(name, value));
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

        // pass env
        for (auto& [n, v] : env) {
            setenv(n.c_str(), v.c_str(), 1);
        }

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

        pollfd pollfds[2] = {
            {.fd = outPipe[0], .events = POLLIN, .revents = 0},
            {.fd = errPipe[0], .events = POLLIN, .revents = 0},
        };

        while (1337) {
            int ret = poll(pollfds, 2, 5000);

            if (ret < 0) {
                if (errno == EINTR)
                    continue;

                return false;
            }

            bool hupd = false;

            for (size_t i = 0; i < 2; ++i) {
                if (pollfds[i].revents & POLLHUP) {
                    hupd = true;
                    break;
                }
            }

            if (hupd)
                break;

            if (pollfds[0].revents & POLLIN) {
                while ((ret = read(outPipe[0], buf.data(), 1023)) > 0) {
                    out += std::string_view{(char*)buf.data(), (size_t)ret};
                }

                buf.fill(0);
            }

            if (pollfds[1].revents & POLLIN) {
                while ((ret = read(errPipe[0], buf.data(), 1023)) > 0) {
                    err += std::string_view{(char*)buf.data(), (size_t)ret};
                }

                buf.fill(0);
            }
        }

        // Final reads. Nonblock, so its ok.
        while ((ret = read(outPipe[0], buf.data(), 1023)) > 0) {
            out += std::string_view{(char*)buf.data(), (size_t)ret};
        }

        buf.fill(0);

        while ((ret = read(errPipe[0], buf.data(), 1023)) > 0) {
            err += std::string_view{(char*)buf.data(), (size_t)ret};
        }

        buf.fill(0);

        close(outPipe[0]);
        close(errPipe[0]);

        // reap child
        waitpid(pid, nullptr, 0);

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
                argsC.emplace_back(strdup(arg.c_str()));
            }

            argsC.emplace_back(nullptr);

            execvp(binary.c_str(), (char* const*)argsC.data());
            _exit(0);
        }
        close(socket[0]);
        if (write(socket[1], &grandchild, sizeof(grandchild)) != sizeof(grandchild)) {
            close(socket[1]);
            _exit(1);
        }
        close(socket[1]);
        _exit(0);
    }

    // run in parent
    close(socket[1]);
    ssize_t bytesRead = read(socket[0], &grandchild, sizeof(grandchild));
    close(socket[0]);

    if (bytesRead != sizeof(grandchild)) {
        waitpid(child, nullptr, 0);
        return false;
    }

    // clear child and leave grandchild to init
    waitpid(child, nullptr, 0);

    grandchildPid = grandchild;

    return true;
}

const std::string& Hyprutils::OS::CProcess::stdOut() {
    return out;
}

const std::string& Hyprutils::OS::CProcess::stdErr() {
    return err;
}

const pid_t& Hyprutils::OS::CProcess::pid() {
    return grandchildPid;
}
