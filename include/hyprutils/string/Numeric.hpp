#pragma once
#include <string_view>
#include <cstdint>
#include <expected>
#include <charconv>
#include <concepts>

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

        const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);

        if (ec == std::errc::invalid_argument)
            return std::unexpected(NUMERIC_PARSE_BAD);
        if (ec == std::errc::result_out_of_range)
            return std::unexpected(NUMERIC_PARSE_OUT_OF_RANGE);
        if (ptr != sv.data() + sv.size())
            return std::unexpected(NUMERIC_PARSE_GARBAGE);

        return value;
    }
};