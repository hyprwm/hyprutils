#include <hyprutils/os/FileDescriptor.hpp>
#include "shared.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace Hyprutils::OS;

int main() {
    std::string     name = "/test_filedescriptors";
    CFileDescriptor fd(shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600));

    int             ret = 0;
    EXPECT(fd.isValid(), true);
    EXPECT(fd.isReadable(), true);

    int flags = fd.getFlags();
    EXPECT(fd.getFlags(), FD_CLOEXEC);
    flags &= ~FD_CLOEXEC;
    fd.setFlags(flags);
    EXPECT(fd.getFlags(), !FD_CLOEXEC);

    CFileDescriptor fd2 = fd.duplicate();
    EXPECT(fd.isValid(), true);
    EXPECT(fd.isReadable(), true);
    EXPECT(fd2.isValid(), true);
    EXPECT(fd2.isReadable(), true);

    CFileDescriptor fd3(fd2.take());
    EXPECT(fd.isValid(), true);
    EXPECT(fd.isReadable(), true);
    EXPECT(fd2.isValid(), false);
    EXPECT(fd2.isReadable(), false);

    // .duplicate default flags is FD_CLOEXEC
    EXPECT(fd3.getFlags(), FD_CLOEXEC);

    fd.reset();
    fd2.reset();
    fd3.reset();

    EXPECT(fd.isReadable(), false);
    EXPECT(fd2.isReadable(), false);
    EXPECT(fd3.isReadable(), false);

    shm_unlink(name.c_str());

    return ret;
}
