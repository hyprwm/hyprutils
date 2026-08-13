#pragma once

namespace Hyprutils::OS {
    /*
     * This is a named flock, Scoped, RAII.
     */
    class CSemaphore {
      public:
        CSemaphore(const char* name);
        ~CSemaphore();

        CSemaphore(const CSemaphore&)            = delete;
        CSemaphore& operator=(const CSemaphore&) = delete;
        CSemaphore(CSemaphore&&)                 = delete;
        CSemaphore& operator=(CSemaphore&&)      = delete;

      private:
        int m_fd = -1;
    };
};
