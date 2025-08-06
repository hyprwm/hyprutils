#pragma once
#include <bit>
#include <utility>
namespace Hyprutils::Memory {
    template <typename To, typename From>
    constexpr To sc(From&& from) noexcept {
        return static_cast<To>(std::forward<From>(from));
    }

    template <typename To, typename From>
    constexpr To cc(From&& from) noexcept {
        return const_cast<To>(std::forward<From>(from));
    }

    template <typename To, typename From>
    constexpr To rc(From&& from) noexcept {
        return reinterpret_cast<To>(std::forward<From>(from));
    }

    template <typename To, typename From>
    constexpr To dc(From&& from) {
        return dynamic_cast<To>(std::forward<From>(from));
    }

    template <typename To, typename From>
    constexpr To bc(const From& from) noexcept {
        return std::bit_cast<To>(from);
    }
}