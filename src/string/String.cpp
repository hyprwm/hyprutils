#include <algorithm>
#include <hyprutils/string/String.hpp>

using namespace Hyprutils::String;

std::string Hyprutils::String::trim(const std::string& in) {
    if (in.empty())
        return in;

    int countBefore = 0;
    while (countBefore < in.length() && std::isspace(in.at(countBefore))) {
        countBefore++;
    }

    int countAfter = 0;
    while (countAfter < in.length() - countBefore && std::isspace(in.at(in.length() - countAfter - 1))) {
        countAfter++;
    }

    std::string result = in.substr(countBefore, in.length() - countBefore - countAfter);

    return result;
}

bool Hyprutils::String::isNumber(const std::string& str, bool allowfloat) {
    if (str.empty())
        return false;

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

            if (str.at(0) == '-')
                return false;

            continue;
        }
    }

    if (str.back() == '.')
        return false;

    return true;
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
