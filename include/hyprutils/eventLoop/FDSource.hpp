#pragma once

#include <expected>
#include <string>

#include "../os/FileDescriptor.hpp"
#include "EventMask.hpp"

namespace Hyprutils::EventLoop {
    class IFDSource {
      public:
        virtual ~IFDSource() = default;

        IFDSource(const IFDSource&)            = delete;
        IFDSource(IFDSource&&)                 = delete;
        IFDSource& operator=(const IFDSource&) = delete;
        IFDSource& operator=(IFDSource&&)      = delete;

        // HUP and ERROR are always reported and cannot be requested in mask.
        virtual std::expected<void, std::string> setMask(FdEventMask mask) = 0;
        virtual void                             remove()                  = 0;
        virtual const OS::CFileDescriptor&       fd() const                = 0;

      private:
        IFDSource() = default;
        friend class CFDSource;
    };
}
