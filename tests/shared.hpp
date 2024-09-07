#pragma once
#include <iostream>

namespace Colors {
    constexpr auto RED     = "\x1b[31m";
    constexpr auto GREEN   = "\x1b[32m";
    constexpr auto YELLOW  = "\x1b[33m";
    constexpr auto BLUE    = "\x1b[34m";
    constexpr auto MAGENTA = "\x1b[35m";
    constexpr auto CYAN    = "\x1b[36m";
    constexpr auto RESET   = "\x1b[0m";
}

#define EXPECT(expr, val)                                                                                                                                                          \
    if (const auto RESULT = expr; RESULT != (val)) {                                                                                                                               \
        std::cout << Colors::RED << "Failed: " << Colors::RESET << #expr << ", expected " << #val << " but got " << RESULT << "\n";                                                \
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
#define EXPECT_MATRIX(mat1, mat2, size, testName)                                                                                                                                  \
    do {                                                                                                                                                                           \
        bool success = true;                                                                                                                                                       \
        for (size_t i = 0; i < size; i++) {                                                                                                                                        \
            if (std::abs((mat1)[i] - (mat2)[i]) >= 1e-5) {                                                                                                                         \
                std::cout << Colors::RED << "Failed: ";                                                                                                                            \
                if (testName && *testName != '\0') {                                                                                                                               \
                    std::cout << testName << " ";                                                                                                                                  \
                }                                                                                                                                                                  \
                std::cout << "Matrices not equal at index " << i << ". Expected " << (mat2)[i] << " but got " << (mat1)[i] << "\n";                                                \
                success = false;                                                                                                                                                   \
                ret     = 1;                                                                                                                                                       \
                break;                                                                                                                                                             \
            }                                                                                                                                                                      \
        }                                                                                                                                                                          \
        if (success) {                                                                                                                                                             \
            std::cout << Colors::GREEN << "Passed: ";                                                                                                                              \
            if (testName && *testName != '\0') {                                                                                                                                   \
                std::cout << testName << " ";                                                                                                                                      \
            }                                                                                                                                                                      \
            std::cout << "Matrices are equal.\n" << Colors::RESET;                                                                                                                 \
        }                                                                                                                                                                          \
        std::cout << "Matrix received: {";                                                                                                                                         \
        for (size_t i = 0; i < size; i++) {                                                                                                                                        \
            std::cout << (mat1)[i];                                                                                                                                                \
            if (i < size - 1) {                                                                                                                                                    \
                std::cout << ", ";                                                                                                                                                 \
            }                                                                                                                                                                      \
        }                                                                                                                                                                          \
        std::cout << "}\n";                                                                                                                                                        \
    } while (0)
