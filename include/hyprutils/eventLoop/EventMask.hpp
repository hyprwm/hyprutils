#pragma once

#include <cstdint>

#include "../macros/Enums.hpp"
#include "../memory/Casts.hpp"

namespace Hyprutils::EventLoop {
    enum class eEventMask : uint8_t {
        EMPTY    = 0,
        READABLE = (1 << 0),
        WRITABLE = (1 << 1),
        HUP      = (1 << 2),
        ERROR    = (1 << 3),
    };

    // NOLINTNEXTLINE SHUT UP
    HYPRUTILS_EXPOSE_ENUM_AS_MASK(eEventMask, FdEventMask);
}
