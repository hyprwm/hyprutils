#include <ranges>
#include <algorithm>
#include <hyprutils/string/String.hpp>
#include <hyprutils/string/ConstVarList.hpp>

using namespace Hyprutils::String;

CConstVarList::CConstVarList(const std::string& in, const size_t lastArgNo, const char delim, const bool removeEmpty) : m_str(in) {
    if (in.empty())
        return;

    size_t idx = 0;
    size_t pos = 0;
    std::ranges::replace_if(m_str, [&](const char& c) { return delim == 's' ? std::isspace(c) : c == delim; }, 0);

    for (const auto& s : m_str | std::views::split(0)) {
        if (removeEmpty && s.empty())
            continue;
        if (++idx == lastArgNo) {
            m_args.emplace_back(trim(std::string_view{m_str}.substr(pos)));
            break;
        }
        pos += s.size() + 1;
        m_args.emplace_back(trim(std::string_view{s.data()}));
    }
}

std::string CConstVarList::join(const std::string& joiner, size_t from, size_t to) const {
    size_t      last = to == 0 ? size() : to;

    std::string rolling;
    for (size_t i = from; i < last; ++i) {
        // cast can be removed once C++26's change to allow this is supported
        rolling += std::string{m_args[i]} + (i + 1 < last ? joiner : "");
    }

    return rolling;
}
