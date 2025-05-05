#include <ranges>
#include <algorithm>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/string/String.hpp>

using namespace Hyprutils::String;

Hyprutils::String::CVarList::CVarList(const std::string& in, const size_t lastArgNo, const char delim, const bool removeEmpty) {
    if (!removeEmpty && in.empty()) {
        m_vArgs.emplace_back("");
        return;
    }

    std::string currentArg;
    bool        escaped = false;
    size_t      idx     = 0;

    for (size_t i = 0; i < in.length(); ++i) {
        char c = in[i];

        // Handle escape character
        if (c == '\\' && !escaped) {
            escaped = true;
            continue;
        }

        // Check if we've hit a delimiter that's not escaped
        bool isDelim = (delim == 's' ? std::isspace(c) : c == delim) && !escaped;

        if (isDelim) {
            if (!removeEmpty || !currentArg.empty()) {
                m_vArgs.emplace_back(trim(currentArg));
                currentArg.clear();
                idx++;
            }

            // If this is the last argument we want to parse separately
            if (idx == lastArgNo - 1) {
                m_vArgs.emplace_back(trim(in.substr(i + 1)));
                return;
            }
        } else {
            currentArg += c;
        }

        escaped = false;
    }

    // Add the last argument if there is one
    if (!removeEmpty || !currentArg.empty()) {
        m_vArgs.emplace_back(trim(currentArg));
    }
}

std::string Hyprutils::String::CVarList::join(const std::string& joiner, size_t from, size_t to) const {
    size_t      last = to == 0 ? size() : to;

    std::string rolling;
    for (size_t i = from; i < last; ++i) {
        rolling += m_vArgs[i] + (i + 1 < last ? joiner : "");
    }

    return rolling;
}
