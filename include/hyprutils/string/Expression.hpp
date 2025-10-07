
#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <type_traits>
#include <expected>
#include <charconv>
#include <cctype>
#include <cmath>

namespace Hyprutils::Expression {

    template <typename T>
    using calc_t = std::conditional_t<std::is_floating_point_v<T>, T, long double>;

    static inline void skip(std::string_view s, size_t& i) {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
            ++i;
    }

    static inline char peek(std::string_view s, size_t i) {
        return i < s.size() ? s[i] : '\0';
    }

    static inline char get(std::string_view s, size_t& i) {
        return i < s.size() ? s[i++] : '\0';
    }

    static inline bool match(std::string_view s, size_t& i, char c) {
        if (peek(s, i) == c) {
            ++i;
            return true;
        }
        return false;
    }

    template <typename T>
    static std::expected<calc_t<T>, std::string> parseExpr(std::string_view, size_t&, const std::unordered_map<std::string, T>&);

    template <typename T>
    static std::expected<calc_t<T>, std::string> parsePrimary(std::string_view s, size_t& i, const std::unordered_map<std::string, T>& vars) {
        skip(s, i);

        if (match(s, i, '(')) {
            auto v = parseExpr<T>(s, i, vars);
            if (!v)
                return v;
            skip(s, i);
            if (!match(s, i, ')'))
                return std::unexpected("Expected ')'");
            return v;
        }

        if (std::isalpha(peek(s, i))) {
            std::string name;
            while (std::isalnum(peek(s, i)))
                name += get(s, i);
            if (auto it = vars.find(name); it != vars.end())
                return static_cast<calc_t<T>>(it->second);
            return std::unexpected("Unknown variable: " + name);
        }

        if (i >= s.size())
            return std::unexpected("Expected number, got '<end of input>'");

        char c = peek(s, i);
        if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.')
            return std::unexpected(std::string("Expected number, got: '") + c + "'");

        calc_t<T> val{};
        double    tmp{};
        auto [ptr, ec] = std::from_chars(s.data() + i, s.data() + s.size(), tmp);
        if (ec != std::errc())
            return std::unexpected("Invalid number");
        val = static_cast<calc_t<T>>(tmp);
        i   = ptr - s.data();

        return val;
    }

    template <typename T>
    static std::expected<calc_t<T>, std::string> parseFactor(std::string_view s, size_t& i, const std::unordered_map<std::string, T>& vars) {
        skip(s, i);
        bool neg = false;
        while (match(s, i, '+') || match(s, i, '-')) {
            if (s[i - 1] == '-')
                neg = !neg;
            skip(s, i);
        }

        auto v = parsePrimary<T>(s, i, vars);
        if (!v)
            return v;

        skip(s, i);
        if (match(s, i, '%'))
            *v /= 100.0;

        if (neg)
            *v = -*v;
        return v;
    }

    template <typename T>
    static std::expected<calc_t<T>, std::string> parseTerm(std::string_view s, size_t& i, const std::unordered_map<std::string, T>& vars) {
        auto lhs = parseFactor<T>(s, i, vars);
        if (!lhs)
            return lhs;

        while (true) {
            skip(s, i);
            char op = peek(s, i);
            if (op != '*' && op != '/')
                break;
            get(s, i);

            auto rhs = parseFactor<T>(s, i, vars);
            if (!rhs)
                return rhs;

            if (op == '*')
                *lhs *= *rhs;
            else {
                if constexpr (std::is_floating_point_v<T>)
                    if (std::abs(*rhs) < 1e-12)
                        return std::unexpected("Division by zero");
                if constexpr (std::is_integral_v<T>)
                    if (*rhs == 0)
                        return std::unexpected("Division by zero");
                *lhs /= *rhs;
            }
        }
        return lhs;
    }

    template <typename T>
    static std::expected<calc_t<T>, std::string> parseExpr(std::string_view s, size_t& i, const std::unordered_map<std::string, T>& vars) {
        auto lhs = parseTerm<T>(s, i, vars);
        if (!lhs)
            return lhs;

        while (true) {
            skip(s, i);
            char op = peek(s, i);
            if (op != '+' && op != '-')
                break;
            get(s, i);

            auto rhs = parseTerm<T>(s, i, vars);
            if (!rhs)
                return rhs;

            if (op == '+')
                *lhs += *rhs;
            else
                *lhs -= *rhs;
        }
        return lhs;
    }

    template <typename T>
    std::expected<T, std::string> eval(std::string_view expr, const std::unordered_map<std::string, T>& vars) {
        size_t i   = 0;
        auto   res = parseExpr<T>(expr, i, vars);
        if (!res)
            return std::unexpected(res.error());
        skip(expr, i);
        if (i != expr.size())
            return std::unexpected("Unexpected trailing characters");
        return static_cast<T>(*res);
    }

} // namespace Hyprutils::Expression
