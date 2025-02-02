#pragma once

#include <fcntl.h>

//NOLINTNEXTLINE
namespace Hyprutils::OS {
    class CFileDescriptor {
      public:
        CFileDescriptor() = default;
        explicit CFileDescriptor(int const fd);
        CFileDescriptor(CFileDescriptor&&) noexcept;
        CFileDescriptor& operator=(CFileDescriptor&&) noexcept;
        ~CFileDescriptor();

        CFileDescriptor(const CFileDescriptor&)            = delete;
        CFileDescriptor& operator=(const CFileDescriptor&) = delete;

        bool             operator==(const CFileDescriptor& rhs) const {
            return m_fd == rhs.m_fd;
        }

        bool            isValid() const;
        int             get() const;
        int             getFlags() const;
        bool            setFlags(int flags);
        int             take();
        void            reset();
        CFileDescriptor duplicate(int flags = F_DUPFD_CLOEXEC) const;

        bool            isReadable() const;
        bool            isClosed() const;

        static bool     isReadable(int fd);
        static bool     isClosed(int fd);

      private:
        int m_fd = -1;
    };
};
