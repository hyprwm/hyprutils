#include <algorithm>

#include <hyprutils/string/VarList2.hpp>
#include <hyprutils/string/String.hpp>

using namespace Hyprutils::String;

CVarList2::CVarList2(std::string&& in, const size_t lastArgNo, const char delim, const bool removeEmpty, const bool allowEscape) : m_inString(std::move(in)) {
    if (!removeEmpty && m_inString.empty())
        return;

    auto                isDelimiter = [&delim](const char& c) { return delim == 's' ? std::isspace(c) : delim == c; };

    size_t              argBegin = 0;
    std::vector<size_t> escapedIndices; // local to the current arg
    for (size_t i = 0; i < m_inString.size(); ++i) {
        const char& c = m_inString[i];

        if (!isDelimiter(c))
            continue;

        if (allowEscape) {
            // we allow escape, so this might be escaped. Check first
            if (i - argBegin == 0)
                continue; // can't be
            const char& previousC = m_inString[i - 1];
            if (i - argBegin == 1) {
                if (previousC == '\\') {
                    escapedIndices.emplace_back(i - argBegin - 1);
                    continue; // escaped
                }
                // fall to breaking, not escaped
            } else {
                const char& prevPreviousC = m_inString[i - 2];
                if (previousC == '\\') {
                    // whether or not escaped, pop char
                    escapedIndices.emplace_back(i - argBegin - 1);

                    if (prevPreviousC != '\\') {
                        // escaped
                        continue;
                    }
                }

                // fall to breaking, not escaped, but mark the \\ to be popped
            }
        }

        // here we found a delimiter and need to break up the string (not escaped)

        if (escapedIndices.empty()) {
            // we didn't escape anything, so we can use inString
            const auto ARG = trim(std::string_view{m_inString}.substr(argBegin, i - argBegin));

            if (!ARG.empty() || !removeEmpty)
                m_args.emplace_back(ARG);
        } else {
            // we escaped something, fixup the string, add to copies, then emplace
            std::string cpy = m_inString.substr(argBegin, i - argBegin);
            for (size_t i = 0; i < escapedIndices.size(); ++i) {
                cpy = cpy.substr(0, escapedIndices[i] - i) + cpy.substr(escapedIndices[i] - i + 1);
            }
            m_copyStrings.emplace_back(std::move(cpy));
            m_args.emplace_back(trim(std::string_view{m_copyStrings.back()}));
        }

        // update next argBegin
        argBegin = i + 1;
        escapedIndices.clear();
    }

    // append anything left
    if (argBegin < m_inString.size()) {
        if (escapedIndices.empty()) {
            // we didn't escape anything, so we can use inString
            const auto ARG = trim(std::string_view{m_inString}.substr(argBegin, m_inString.size() - argBegin));

            if (!ARG.empty() || !removeEmpty)
                m_args.emplace_back(ARG);
        } else {
            // we escaped something, fixup the string, add to copies, then emplace
            std::string cpy = m_inString.substr(argBegin, m_inString.size() - argBegin);
            for (size_t i = 0; i < escapedIndices.size(); ++i) {
                cpy = cpy.substr(0, escapedIndices[i] - i) + cpy.substr(escapedIndices[i] - i + 1);
            }
            m_copyStrings.emplace_back(std::move(cpy));
            m_args.emplace_back(trim(std::string_view{m_copyStrings.back()}));
        }
    }
}

std::string CVarList2::join(const std::string& joiner, size_t from, size_t to) const {
    if (to == 0 || to <= from)
        return "";

    std::string roll;
    for (size_t i = from; i < to; ++i) {
        roll += m_args[i];
        if (i + 1 < to)
            roll += joiner;
    }
    return roll;
}

void CVarList2::append(std::string&& arg) {
    m_copyStrings.emplace_back(std::move(arg));
    m_args.emplace_back(m_copyStrings.back());
}

bool CVarList2::contains(const std::string& el) {
    return std::ranges::any_of(m_args, [&el](const auto& e) { return e == el; });
}
