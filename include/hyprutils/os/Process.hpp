#pragma once

#include <string>
#include <vector>
#include <utility>
#include <sys/types.h>

namespace Hyprutils {
    namespace OS {
        class CProcess {
          public:
            /* Creates a process object, doesn't run yet */
            CProcess(const std::string& binary_, const std::vector<std::string>& args_);
            ~CProcess();

            CProcess(CProcess&)                  = delete;
            CProcess(CProcess&&)                 = delete;
            CProcess(const CProcess&&)           = delete;
            CProcess(const CProcess&)            = delete;
            CProcess& operator=(const CProcess&) = delete;
            CProcess& operator=(CProcess&&)      = delete;

            void      addEnv(const std::string& name, const std::string& value);

            // only for async, sync doesn't make sense
            void setStdinFD(int fd);
            // only for async, sync doesn't make sense
            void setStdoutFD(int fd);
            // only for async, sync doesn't make sense
            void setStderrFD(int fd);

            /* Run the process, synchronously, get the stdout and stderr. False on fail */
            bool runSync();

            /* Run the process, asynchronously. This will detach the process from this object (and process) and let it live a happy life. False on fail. */
            bool runAsync();

            // only populated when ran sync
            const std::string& stdOut();
            const std::string& stdErr();

            pid_t              pid();

            // only for sync
            int exitCode();

          private:
            struct impl;
            impl* m_impl;
        };
    }
}
