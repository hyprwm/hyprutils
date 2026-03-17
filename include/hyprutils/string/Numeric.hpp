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