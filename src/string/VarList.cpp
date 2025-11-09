#include <ranges>
#include <algorithm>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/string/String.hpp>

using namespace Hyprutils::String;

Hyprutils::String::CVarList::CVarList(const std::string& in, const size_t lastArgNo, const char delim, const bool removeEmpty) {
    if (!removeEmpty && in.empty())
        m_vArgs.emplace_back("");

    std::string args{in};
    size_t      idx = 0;
    size_t      pos = 0;
    std::ranges::replace_if(args, [&](const char& c) { return delim == 's' ? std::isspace(c) : c == delim; }, 0);

    for (const auto& s : args | std::views::split(0)) {
        if (removeEmpty && s.empty())
            continue;
        if (++idx == lastArgNo) {
            m_vArgs.emplace_back(trim(in.substr(pos)));
            break;
        }
        pos += s.size() + 1;
        m_vArgs.emplace_back(trim(std::string{s.data()}));
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

#ifdef HU_UNIT_TESTS

#include <gtest/gtest.h>

TEST(String, varlist) {
    CVarList list("hello    world!", 0, 's', true);
    EXPECT_EQ(list[0], "hello");
    EXPECT_EQ(list[1], "world!");
}

#endif
