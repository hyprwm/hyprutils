#pragma once
#include <string_view>
#include <cstdint>
#include <expected>
#include <charconv>
#include <concepts>

#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 200000
#include <string>
#include <cstdlib>
#include <cerrno>
#endif

namespace Hyprutils::String {

    enum eNumericParseResult : uint8_t {
        NUMERIC_PARSE_OK = 0,
        NUMERIC_PARSE_GARBAGE,
        NUMERIC_PARSE_BAD,
        NUMERIC_PARSE_OUT_OF_RANGE
    };

    template <typename T>
        requires std::integral<T> || std::floating_point<T>
    std::expected<T, eNumericParseResult> strToNumber(std::string_view sv) {
        if (sv.empty())
            return std::unexpected(NUMERIC_PARSE_BAD);

        T value{};

        if constexpr (std::integral<T>) {
            if (sv.size() >= 2 && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X')) {
                if (sv.size() == 2)
                    return std::unexpected(NUMERIC_PARSE_BAD);
                const auto hex       = sv.substr(2);
                const auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), value, 16);

                if (ec == std::errc::invalid_argument)
                    return std::unexpected(NUMERIC_PARSE_BAD);
                if (ec == std::errc::result_out_of_range)
                    return std::unexpected(NUMERIC_PARSE_OUT_OF_RANGE);
                if (ptr != hex.data() + hex.size())
                    return std::unexpected(NUMERIC_PARSE_GARBAGE);

                return value;
            }
        }

#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 200000
        // libc++ < 20 does not implement std::from_chars for floating point types
        if constexpr (std::floating_point<T>) {
            std::string_view ts = sv;
            if (ts.starts_with('+') || ts.starts_with('-'))
                ts.remove_prefix(1);
            if (ts.size() >= 2 && ts[0] == '0' && (ts[1] == 'x' || ts[1] == 'X'))
                return std::unexpected(NUMERIC_PARSE_GARBAGE);

            std::string s{sv};
            char*       endptr = nullptr;
            errno              = 0;

            if constexpr (std::same_as<T, float>)
                value = std::strtof(s.c_str(), &endptr);
            else if constexpr (std::same_as<T, double>)
                value = std::strtod(s.c_str(), &endptr);
            else
                value = std::strtold(s.c_str(), &endptr);

            if (endptr == s.c_str())
                return std::unexpected(NUMERIC_PARSE_BAD);
            if (errno == ERANGE)
                return std::unexpected(NUMERIC_PARSE_OUT_OF_RANGE);
            if (endptr != s.c_str() + s.size())
                return std::unexpected(NUMERIC_PARSE_GARBAGE);

            return value;
        } else {
            const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);

            if (ec == std::errc::invalid_argument)
                return std::unexpected(NUMERIC_PARSE_BAD);
            if (ec == std::errc::result_out_of_range)
                return std::unexpected(NUMERIC_PARSE_OUT_OF_RANGE);
            if (ptr != sv.data() + sv.size())
                return std::unexpected(NUMERIC_PARSE_GARBAGE);

            return value;
        }
#else
        const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);

        if (ec == std::errc::invalid_argument)
            return std::unexpected(NUMERIC_PARSE_BAD);
        if (ec == std::errc::result_out_of_range)
            return std::unexpected(NUMERIC_PARSE_OUT_OF_RANGE);
        if (ptr != sv.data() + sv.size())
            return std::unexpected(NUMERIC_PARSE_GARBAGE);

        return value;
#endif
    }
};
