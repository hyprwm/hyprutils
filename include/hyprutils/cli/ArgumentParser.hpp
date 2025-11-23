#pragma once

#include <span>
#include <string>
#include <string_view>
#include <optional>
#include <expected>
#include "../memory/UniquePtr.hpp"

namespace Hyprutils::CLI {
    class CArgumentParserImpl;

    class CArgumentParser {
      public:
        CArgumentParser(const std::span<const char*>& args);
        ~CArgumentParser() = default;

        std::expected<void, std::string> registerBoolOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description);
        std::expected<void, std::string> registerIntOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description);
        std::expected<void, std::string> registerFloatOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description);
        std::expected<void, std::string> registerStringOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description);

        std::optional<bool>              getBool(const std::string_view& name);
        std::optional<int>               getInt(const std::string_view& name);
        std::optional<float>             getFloat(const std::string_view& name);
        std::optional<std::string_view>  getString(const std::string_view& name);

        // commence the parsing after registering
        std::expected<void, std::string> parse();

        std::string                      getDescription(const std::string_view& header, std::optional<size_t> maxWidth = {});

      private:
        Memory::CUniquePointer<CArgumentParserImpl> m_impl;
    };
};