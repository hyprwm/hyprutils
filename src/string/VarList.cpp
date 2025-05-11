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

    std::vector<std::pair<size_t, size_t>> argIndices;
    size_t                                 currentStart = 0;
    size_t                                 idx          = 0;

    for (size_t i = 0; i < in.length(); ++i) {
        if (handleEscape && in[i] == '\\' && i + 1 < in.length()) {
            i++;
            continue;
        }

        const char c       = in[i];
        const bool isDelim = (delim == 's' ? std::isspace(c) : c == delim);

        if (isDelim) {
            if (!removeEmpty || i > currentStart) {
                argIndices.emplace_back(currentStart, i);
                idx++;
            }

            currentStart = i + 1;

            if (idx == lastArgNo - 1) {
                argIndices.emplace_back(i + 1, in.length());
                break;
            }
        }
    }

    if (currentStart < in.length() && (!removeEmpty || currentStart < in.length()))
        argIndices.emplace_back(currentStart, in.length());

    m_vArgs.reserve(argIndices.size());
    for (const auto& [start, end] : argIndices) {
        if (handleEscape) {
            std::string segment = in.substr(start, end - start);
            replaceInString(segment, "\\", "");
            m_vArgs.emplace_back(trim(segment));
        } else
            m_vArgs.emplace_back(trim(in.substr(start, end - start)));
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
