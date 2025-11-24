#include "ArgumentParser.hpp"

#include <format>
#include <vector>
#include <algorithm>

#include <hyprutils/string/String.hpp>
#include <hyprutils/memory/Casts.hpp>

using namespace Hyprutils::CLI;
using namespace Hyprutils::Memory;
using namespace Hyprutils::String;
using namespace Hyprutils;

CArgumentParser::CArgumentParser(const std::span<const char*>& args) : m_impl(makeUnique<CArgumentParserImpl>(args)) {
    ;
}

std::expected<void, std::string> CArgumentParser::registerBoolOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description) {
    return m_impl->registerOption(name, abbrev, description, ARG_TYPE_BOOL);
}

std::expected<void, std::string> CArgumentParser::registerIntOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description) {
    return m_impl->registerOption(name, abbrev, description, ARG_TYPE_INT);
}

std::expected<void, std::string> CArgumentParser::registerFloatOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description) {
    return m_impl->registerOption(name, abbrev, description, ARG_TYPE_FLOAT);
}

std::expected<void, std::string> CArgumentParser::registerStringOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description) {
    return m_impl->registerOption(name, abbrev, description, ARG_TYPE_STR);
}

std::optional<bool> CArgumentParser::getBool(const std::string_view& name) {
    auto ref = m_impl->getValue(name);

    if (ref == m_impl->m_values.end())
        return std::nullopt;

    if (const auto pval = std::get_if<bool>(&ref->val); pval)
        return *pval;

    return std::nullopt;
}

std::optional<int> CArgumentParser::getInt(const std::string_view& name) {
    auto ref = m_impl->getValue(name);

    if (ref == m_impl->m_values.end())
        return std::nullopt;

    if (const auto pval = std::get_if<int>(&ref->val); pval)
        return *pval;

    return std::nullopt;
}

std::optional<float> CArgumentParser::getFloat(const std::string_view& name) {
    auto ref = m_impl->getValue(name);

    if (ref == m_impl->m_values.end())
        return std::nullopt;

    if (const auto pval = std::get_if<float>(&ref->val); pval)
        return *pval;

    return std::nullopt;
}

std::optional<std::string_view> CArgumentParser::getString(const std::string_view& name) {
    auto ref = m_impl->getValue(name);

    if (ref == m_impl->m_values.end())
        return std::nullopt;

    if (const auto pval = std::get_if<std::string>(&ref->val); pval)
        return *pval;

    return std::nullopt;
}

std::string CArgumentParser::getDescription(const std::string_view& header, std::optional<size_t> maxWidth) {
    return m_impl->getDescription(header, maxWidth);
}

std::expected<void, std::string> CArgumentParser::parse() {
    return m_impl->parse();
}

CArgumentParserImpl::CArgumentParserImpl(const std::span<const char*>& args) {
    m_argv.reserve(args.size());
    for (const auto& a : args) {
        m_argv.emplace_back(a);
    }
}

std::vector<SArgumentKey>::iterator CArgumentParserImpl::getValue(const std::string_view& sv) {
    if (sv.empty())
        return m_values.end();
    auto it = std::ranges::find_if(m_values, [&sv](const auto& e) { return e.full == sv || e.abbrev == sv; });
    return it;
}

std::expected<void, std::string> CArgumentParserImpl::registerOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description,
                                                                     eArgumentType type) {
    if (name.empty())
        return std::unexpected("Name cannot be empty");

    if (getValue(name) != m_values.end() || getValue(abbrev) != m_values.end())
        return std::unexpected("Value already exists");

    m_values.emplace_back(SArgumentKey{
        .full    = std::string{name},
        .abbrev  = std::string{abbrev},
        .desc    = std::string{description},
        .argType = type,
        .val     = std::monostate{},
    });

    return {};
}

std::expected<void, std::string> CArgumentParserImpl::parse() {
    // walk the args
    for (size_t i = 1; i < m_argv.size(); ++i) {
        auto        val = m_values.end();
        const auto& arg = m_argv.at(i);

        if (arg.starts_with("--"))
            val = getValue(std::string_view{arg}.substr(2));
        else if (arg.starts_with('-'))
            val = getValue(std::string_view{arg}.substr(1));
        else
            return std::unexpected(std::format("Invalid element found while parsing: {}", arg));

        if (val == m_values.end())
            return std::unexpected(std::format("Invalid argument found while parsing: {}", arg));

        switch (val->argType) {
            case ARG_TYPE_BOOL: {
                val->val = true;
                break;
            }
            case ARG_TYPE_INT: {
                if (i + 1 >= m_argv.size())
                    return std::unexpected(std::format("Failed parsing argument {}, no value supplied", arg));
                const auto& next = std::string{m_argv.at(++i)};
                if (!isNumber(next))
                    return std::unexpected(std::format("Failed parsing argument {}, value {} is not an int", arg, next));
                try {
                    val->val = sc<int>(std::stoi(next));
                } catch (...) { return std::unexpected(std::format("Failed parsing argument {}, value {} is not an int", arg, next)); }
                break;
            }
            case ARG_TYPE_FLOAT: {
                if (i + 1 >= m_argv.size())
                    return std::unexpected(std::format("Failed parsing argument {}, no value supplied", arg));
                const auto& next = std::string{m_argv.at(++i)};
                if (!isNumber(next, true))
                    return std::unexpected(std::format("Failed parsing argument {}, value {} is not a float", arg, next));
                try {
                    val->val = sc<float>(std::stof(next));
                } catch (...) { return std::unexpected(std::format("Failed parsing argument {}, value {} is not a float", arg, next)); }
                break;
            }
            case ARG_TYPE_STR: {
                if (i + 1 >= m_argv.size())
                    return std::unexpected(std::format("Failed parsing argument {}, no value supplied", arg));
                val->val = std::string{m_argv.at(++i)};
                break;
            }

            case ARG_TYPE_END: break;
        }
    }

    return {};
}

std::string CArgumentParserImpl::getDescription(const std::string_view& header, std::optional<size_t> maxWidth) {

    const size_t                                          MAX_COLS = maxWidth.value_or(80);
    const std::string                                     PAD_STR  = "                                                                                    ";

    constexpr const std::array<const char*, ARG_TYPE_END> TYPE_STRS = {
        "",        // bool
        "[int]",   // int
        "[float]", // float
        "[str]",   // str
    };

    //
    auto wrap = [](const std::string_view& str, size_t maxW) -> std::vector<std::string_view> {
        std::vector<std::string_view> result;

        // walk word by word
        size_t nextSpacePos = 0, lastBreakPos = 0;
        while (true) {
            size_t lastSpacePos = nextSpacePos;
            nextSpacePos        = str.find(' ', nextSpacePos + 1);

            if (nextSpacePos == std::string::npos)
                break;

            if (nextSpacePos - lastBreakPos > maxW) {
                if (lastSpacePos - lastBreakPos <= maxW) {
                    // break
                    result.emplace_back(str.substr(lastBreakPos, lastSpacePos - lastBreakPos));
                    lastBreakPos = lastSpacePos + 1;
                } else {
                    while (lastSpacePos - lastBreakPos > maxW) {
                        // break
                        result.emplace_back(str.substr(lastBreakPos, maxW));
                        lastBreakPos += maxW;
                    }
                }
                continue;
            }
        }

        result.emplace_back(str.substr(lastBreakPos));

        return result;
    };

    auto pad = [&PAD_STR](size_t len) -> std::string_view {
        if (len >= PAD_STR.size())
            return PAD_STR;
        return std::string_view{PAD_STR}.substr(0, len);
    };

    std::string rolling;
    rolling += std::format("┏ {}\n", header);
    rolling += "┣";
    for (size_t i = 0; i < MAX_COLS; ++i) {
        rolling += "━";
    }
    rolling += "┓\n";

    // get max widths
    size_t maxArgWidth = 0, maxShortWidth = 0;
    for (const auto& v : m_values) {
        maxShortWidth = std::max(maxShortWidth, v.abbrev.size() + 4 + std::string_view{TYPE_STRS[v.argType]}.length());
        maxArgWidth   = std::max(maxArgWidth, v.full.size() + 3);
    }

    // write the table
    for (const auto& v : m_values) {
        size_t lenUsed = 0;
        rolling += "┣ --" + v.full;
        lenUsed += 3 + v.full.size();
        rolling += pad(maxArgWidth - lenUsed);
        lenUsed = maxArgWidth;

        rolling += " -" + v.abbrev;
        lenUsed += 2 + v.abbrev.size();
        rolling += " ";
        rolling += TYPE_STRS[v.argType];
        lenUsed += std::string_view{TYPE_STRS[v.argType]}.length() + 1;
        rolling += pad(maxArgWidth + maxShortWidth - lenUsed);
        lenUsed = maxArgWidth + maxShortWidth;

        rolling += " | ";
        lenUsed += 3;

        const auto ROWS = wrap(v.desc, MAX_COLS - lenUsed);

        const auto LEN_START_DESC = lenUsed;

        rolling += ROWS[0];
        lenUsed += ROWS[0].size();
        rolling += pad(MAX_COLS - lenUsed);
        rolling += "┃\n";

        for (size_t i = 1; i < ROWS.size(); ++i) {
            lenUsed = LEN_START_DESC;
            rolling += "┣";
            rolling += pad(LEN_START_DESC);
            rolling += ROWS[i];
            lenUsed += ROWS[i].size();
            rolling += pad(MAX_COLS - lenUsed);
            rolling += "┃\n";
        }
    }

    rolling += "┗";
    for (size_t i = 0; i < MAX_COLS; ++i) {
        rolling += "━";
    }
    rolling += "┛\n";

    return rolling;
}
