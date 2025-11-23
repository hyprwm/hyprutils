#include <algorithm>
#include <hyprutils/string/String.hpp>

using namespace Hyprutils::String;

std::string Hyprutils::String::trim(const std::string& in) {
    if (in.empty())
        return in;

    size_t countBefore = 0;
    while (countBefore < in.length() && std::isspace(in.at(countBefore))) {
        countBefore++;
    }

    size_t countAfter = 0;
    while (countAfter < in.length() - countBefore && std::isspace(in.at(in.length() - countAfter - 1))) {
        countAfter++;
    }

    std::string result = in.substr(countBefore, in.length() - countBefore - countAfter);

    return result;
}

std::string_view Hyprutils::String::trim(const std::string_view& sv) {
    if (sv.empty())
        return sv;

    size_t countBefore = 0;
    while (countBefore < sv.length() && std::isspace(sv.at(countBefore))) {
        countBefore++;
    }

    size_t countAfter = 0;
    while (countAfter < sv.length() - countBefore && std::isspace(sv.at(sv.length() - countAfter - 1))) {
        countAfter++;
    }

    return sv.substr(countBefore, sv.length() - countBefore - countAfter);
}

std::string Hyprutils::String::trim(const char* in) {
    return trim(std::string{in});
}

bool Hyprutils::String::isNumber(const std::string& str, bool allowfloat) {
    if (str.empty())
        return false;

    bool decimalParsed = false;

    for (size_t i = 0; i < str.length(); ++i) {
        const char& c = str.at(i);

        if (i == 0 && str.at(i) == '-') {
            // only place where we allow -
            continue;
        }

        if (!isdigit(c)) {
            if (!allowfloat)
                return false;

            if (c != '.')
                return false;

            if (i == 0)
                return false;

            if (decimalParsed)
                return false;

            decimalParsed = true;

            continue;
        }
    }

    return isdigit(str.back()) != 0;
}

void Hyprutils::String::replaceInString(std::string& string, const std::string& what, const std::string& to) {
    if (string.empty())
        return;
    size_t pos = 0;
    while ((pos = string.find(what, pos)) != std::string::npos) {
        string.replace(pos, what.length(), to);
        pos += to.length();
    }
}

bool Hyprutils::String::truthy(const std::string_view& in) {
    if (in == "1")
        return true;

    if (in == "0")
        return false;

    std::string lower = std::string{in};
    std::ranges::transform(lower, lower.begin(), ::tolower);

    return lower.starts_with("true") || lower.starts_with("yes") || lower.starts_with("on");
}
