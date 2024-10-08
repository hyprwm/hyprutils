#pragma once
#include <iostream>

namespace Colors {
    constexpr const char* RED     = "\x1b[31m";
    constexpr const char* GREEN   = "\x1b[32m";
    constexpr const char* YELLOW  = "\x1b[33m";
    constexpr const char* BLUE    = "\x1b[34m";
    constexpr const char* MAGENTA = "\x1b[35m";
    constexpr const char* CYAN    = "\x1b[36m";
    constexpr const char* RESET   = "\x1b[0m";
};

#define EXPECT(expr, val)                                                                                                                                                          \
    if (const auto RESULT = expr; RESULT != (val)) {                                                                                                                               \
        std::cout << Colors::RED << "Failed: " << Colors::RESET << #expr << ", expected " << val << " but got " << RESULT << "\n";                                                 \
        ret = 1;                                                                                                                                                                   \
    } else {                                                                                                                                                                       \
        std::cout << Colors::GREEN << "Passed " << Colors::RESET << #expr << ". Got " << val << "\n";                                                                              \
    }
#define EXPECT_VECTOR2D(expr, val)                                                                                                                                                 \
    do {                                                                                                                                                                           \
        const auto& RESULT   = expr;                                                                                                                                               \
        const auto& EXPECTED = val;                                                                                                                                                \
        if (!(std::abs(RESULT.x - EXPECTED.x) < 1e-6 && std::abs(RESULT.y - EXPECTED.y) < 1e-6)) {                                                                                 \
            std::cout << Colors::RED << "Failed: " << Colors::RESET << #expr << ", expected (" << EXPECTED.x << ", " << EXPECTED.y << ") but got (" << RESULT.x << ", "            \
                      << RESULT.y << ")\n";                                                                                                                                        \
            ret = 1;                                                                                                                                                               \
        } else {                                                                                                                                                                   \
            std::cout << Colors::GREEN << "Passed " << Colors::RESET << #expr << ". Got (" << RESULT.x << ", " << RESULT.y << ")\n";                                               \
        }                                                                                                                                                                          \
    } while (0)
