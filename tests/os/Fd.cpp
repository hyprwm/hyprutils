#include <hyprutils/os/FileDescriptor.hpp>

#include <gtest/gtest.h>
#include <sys/mman.h>

using namespace Hyprutils::OS;

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