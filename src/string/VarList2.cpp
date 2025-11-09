#include <algorithm>

#include <hyprutils/string/VarList2.hpp>
#include <hyprutils/string/String.hpp>

using namespace Hyprutils::String;

CVarList2::CVarList2(std::string&& in, const size_t lastArgNo, const char delim, const bool removeEmpty, const bool allowEscape) : m_inString(std::move(in)) {
    if (m_inString.empty())
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
            if (i - argBegin != 0) {
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
                // fall to breaking, couldn't be escaped
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
        to = m_args.size();

    std::string roll;
    for (size_t i = from; i < to && i < m_args.size(); ++i) {
        roll += m_args[i];
        if (i + 1 < to && i + 1 < m_args.size())
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

#ifdef HU_UNIT_TESTS

#include <gtest/gtest.h>

TEST(String, varlist2) {
    CVarList2 varList2("0 set", 2, ' ');
    EXPECT_EQ(varList2[0], "0");
    EXPECT_EQ(varList2[1], "set");

    varList2.append("Hello");

    EXPECT_EQ(varList2[1], "set");
    EXPECT_EQ(varList2[2], "Hello");
    EXPECT_EQ(varList2[3], "");
    EXPECT_EQ(varList2.contains("set"), true);
    EXPECT_EQ(varList2.contains("sett"), false);
    EXPECT_EQ(varList2.contains(""), false);
    EXPECT_EQ(varList2.size(), 3);

    CVarList2 varList2B("hello, world\\, ok?", 0, ',', true, true);
    EXPECT_EQ(varList2B[0], "hello");
    EXPECT_EQ(varList2B[1], "world, ok?");

    CVarList2 varList2C("hello, , ok?", 0, ',', true, true);
    EXPECT_EQ(varList2C[0], "hello");
    EXPECT_EQ(varList2C[1], "ok?");

    CVarList2 varList2D("\\\\, , ok?", 0, ',', true, true);
    EXPECT_EQ(varList2D[0], "\\");
    EXPECT_EQ(varList2D[1], "ok?");

    CVarList2 varList2E("\\, , ok?", 0, ',', true, true);
    EXPECT_EQ(varList2E[0], ",");
    EXPECT_EQ(varList2E[1], "ok?");

    CVarList2 varList2F("Hello, world\\\\, ok?", 0, ',', true, true);
    EXPECT_EQ(varList2F[0], "Hello");
    EXPECT_EQ(varList2F[1], "world\\");
    EXPECT_EQ(varList2F[2], "ok?");

    CVarList2 varList2G("Hello,\\, ok?", 0, ',', true, true);
    EXPECT_EQ(varList2G[0], "Hello");
    EXPECT_EQ(varList2G[1], ", ok?");

    CVarList2 varList2H("Hello,\\\\, ok?", 0, ',', true, true);
    EXPECT_EQ(varList2H[0], "Hello");
    EXPECT_EQ(varList2H[1], "\\");
    EXPECT_EQ(varList2H[2], "ok?");

    CVarList2 varList2I("Hello,\\, ok?", 0, ',', true, false);
    EXPECT_EQ(varList2I[0], "Hello");
    EXPECT_EQ(varList2I[1], "\\");
    EXPECT_EQ(varList2I[2], "ok?");

    CVarList2 varList2J("", 0, ',', true, false);
    EXPECT_EQ(varList2J[0], "");

    CVarList2 varList2K(",\\, ok?", 0, ',', true);
    EXPECT_EQ(varList2K[0], ", ok?");

    CVarList2 varList2L("Hello, world", 0, ',', true);
    EXPECT_EQ(varList2L.join(" "), "Hello world");
    EXPECT_EQ(varList2L.join(" ", 0, 1000), "Hello world");
    EXPECT_EQ(varList2L.join(" ", 0, 1), "Hello");
}

#endif
