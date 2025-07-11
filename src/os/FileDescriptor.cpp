#include <cstdlib>
#include <hyprutils/os/FileDescriptor.hpp>
#include <fcntl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <utility>

using namespace Hyprutils::OS;

CFileDescriptor::CFileDescriptor(int const fd) : m_fd(fd) {}

CFileDescriptor::CFileDescriptor(CFileDescriptor&& other) : m_fd(std::exchange(other.m_fd, -1)) {}

CFileDescriptor& CFileDescriptor::operator=(CFileDescriptor&& other) {
    if (this == &other) // Shit will go haywire if there is duplicate ownership
        abort();

    reset();
    m_fd = std::exchange(other.m_fd, -1);
    return *this;
}

CFileDescriptor::~CFileDescriptor() {
    reset();
}

bool CFileDescriptor::isValid() const {
    return m_fd != -1;
}

int CFileDescriptor::get() const {
    return m_fd;
}

int CFileDescriptor::getFlags() const {
    return fcntl(m_fd, F_GETFD);
}

bool CFileDescriptor::setFlags(int flags) {
    return fcntl(m_fd, F_SETFD, flags) != -1;
}

int CFileDescriptor::take() {
    return std::exchange(m_fd, -1);
}

void CFileDescriptor::reset() {
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

CFileDescriptor CFileDescriptor::duplicate(int flags) const {
    if (m_fd == -1)
        return {};

    int new_fd;
#ifdef F_DUPFD_CLOEXEC
    new_fd = fcntl(m_fd, flags, 0);
#else
    new_fd = fcntl(m_fd, flags | FD_CLOEXEC, 0);
#endif

    return CFileDescriptor{new_fd};
}

bool CFileDescriptor::isClosed() const {
    return isClosed(m_fd);
}

bool CFileDescriptor::isReadable() const {
    return isReadable(m_fd);
}

bool CFileDescriptor::isClosed(int fd) {
    pollfd pfd = {
        .fd      = fd,
        .events  = POLLIN,
        .revents = 0,
    };

    if (poll(&pfd, 1, 0) < 0)
        return true;

    return pfd.revents & (POLLHUP | POLLERR);
}

bool CFileDescriptor::isReadable(int fd) {
    pollfd pfd = {.fd = fd, .events = POLLIN, .revents = 0};

    return poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN);
}
