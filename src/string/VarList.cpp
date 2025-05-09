#include <ranges>
#include <algorithm>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/string/String.hpp>

using namespace Hyprutils::String;

// Original constructor calls the extended one with handleEscape = false
Hyprutils::String::CVarList::CVarList(const std::string& in, const size_t lastArgNo, const char delim, const bool removeEmpty) :
    CVarList(in, lastArgNo, delim, removeEmpty, false) {}

// Extended constructor with escape handling parameter
Hyprutils::String::CVarList::CVarList(const std::string& in, const size_t lastArgNo, const char delim, const bool removeEmpty, const bool handleEscape) {
    if (!removeEmpty && in.empty()) {
        m_vArgs.emplace_back("");
        return;
    }

    std::string currentArg;
    bool        escaped = false;
    size_t      idx     = 0;

    for (size_t i = 0; i < in.length(); ++i) {

        // Handle escape character if enabled
        if (handleEscape && in[i] == '\\') {
            escaped = true;
            i++;
        }

        char c = in[i];

        // Determine if this char is a delimiter (respect escape setting)
        const bool isDelim = (delim == 's' ? std::isspace(c) : c == delim) && !(handleEscape && escaped);

        if (isDelim) {
            if (!removeEmpty || !currentArg.empty()) {
                m_vArgs.emplace_back(trim(currentArg));
                currentArg.clear();
                idx++;
            }

            if (idx == lastArgNo - 1) {
                m_vArgs.emplace_back(trim(in.substr(i + 1)));
                return;
            }
        } else
            currentArg += c;

        if (handleEscape)
            escaped = false;
    }

    if (!removeEmpty || !currentArg.empty())
        m_vArgs.emplace_back(trim(currentArg));
}

std::string Hyprutils::String::CVarList::join(const std::string& joiner, size_t from, size_t to) const {
    size_t      last = to == 0 ? size() : to;

    std::string rolling;
    for (size_t i = from; i < last; ++i) {
        rolling += m_vArgs[i] + (i + 1 < last ? joiner : "");
    }

    return rolling;
}
