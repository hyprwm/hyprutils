#include <hyprutils/os/Semaphore.hpp>

#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <format>

using namespace Hyprutils;
using namespace Hyprutils::OS;

CSemaphore::CSemaphore(const char* name) {
    m_fd = shm_open(std::format("/hu_{}", name).c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);

    if (m_fd == -1)
        return;

    while (flock(m_fd, LOCK_EX) == -1) {
        if (errno != EINTR) {
            close(m_fd);
            m_fd = -1;
            return;
        }
    }
}

CSemaphore::~CSemaphore() {
    if (m_fd != -1)
        close(m_fd);
}
