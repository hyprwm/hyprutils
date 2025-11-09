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

    return CFileDescriptor{fcntl(m_fd, flags, 0)};
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

#ifdef HU_UNIT_TESTS

#include <gtest/gtest.h>
#include <sys/mman.h>

TEST(OS, fd) {
    std::string     name = "/test_filedescriptors";
    CFileDescriptor fd(shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600));

    EXPECT_EQ(fd.isValid(), true);
    EXPECT_EQ(fd.isReadable(), true);

    int flags = fd.getFlags();
    EXPECT_EQ(fd.getFlags(), FD_CLOEXEC);
    flags &= ~FD_CLOEXEC;
    fd.setFlags(flags);
    EXPECT_EQ(fd.getFlags(), !FD_CLOEXEC);

    CFileDescriptor fd2 = fd.duplicate();
    EXPECT_EQ(fd.isValid(), true);
    EXPECT_EQ(fd.isReadable(), true);
    EXPECT_EQ(fd2.isValid(), true);
    EXPECT_EQ(fd2.isReadable(), true);

    CFileDescriptor fd3(fd2.take());
    EXPECT_EQ(fd.isValid(), true);
    EXPECT_EQ(fd.isReadable(), true);
    EXPECT_EQ(fd2.isValid(), false);
    EXPECT_EQ(fd2.isReadable(), false);

    // .duplicate default flags is FD_CLOEXEC
    EXPECT_EQ(fd3.getFlags(), FD_CLOEXEC);

    fd.reset();
    fd2.reset();
    fd3.reset();

    EXPECT_EQ(fd.isReadable(), false);
    EXPECT_EQ(fd2.isReadable(), false);
    EXPECT_EQ(fd3.isReadable(), false);

    shm_unlink(name.c_str());
}

#endif
