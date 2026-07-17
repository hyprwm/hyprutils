#pragma once

#include <exception>
#include <iostream>

#define HYPRUTILS_ASSERT(cond)                                                                                                                                                     \
    do {                                                                                                                                                                           \
        if (!(cond)) {                                                                                                                                                             \
            std::cerr << "Assertion failed: " << #cond << " @ " << __FILE__ << ":" << __LINE__ << "\n";                                                                            \
            std::terminate();                                                                                                                                                      \
        }                                                                                                                                                                          \
    } while (0)

#define HYPRUTILS_ASSERT_MSG(cond, msg)                                                                                                                                            \
    do {                                                                                                                                                                           \
        if (!(cond)) {                                                                                                                                                             \
            std::cerr << "Assertion failed: " << #cond << " (" << msg << ") @ " << __FILE__ << ":" << __LINE__ << "\n";                                                            \
            std::terminate();                                                                                                                                                      \
        }                                                                                                                                                                          \
    } while (0)
