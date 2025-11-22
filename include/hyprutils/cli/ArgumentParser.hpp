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

        std::expected<void, std::string> registerBoolOption(std::string&& name, std::string&& abbrev, std::string&& description);
        std::expected<void, std::string> registerIntOption(std::string&& name, std::string&& abbrev, std::string&& description);
        std::expected<void, std::string> registerFloatOption(std::string&& name, std::string&& abbrev, std::string&& description);
        std::expected<void, std::string> registerStringOption(std::string&& name, std::string&& abbrev, std::string&& description);

        std::optional<bool>              getBool(const char* name);
        std::optional<int>               getInt(const char* name);
        std::optional<float>             getFloat(const char* name);
        std::optional<std::string_view>  getString(const char* name);

        // commence the parsing after registering
        std::expected<void, std::string> parse();

        std::string                      getDescription(const std::string_view& header, std::optional<size_t> maxWidth = {});

      private:
        Memory::CUniquePointer<CArgumentParserImpl> m_impl;
    };
};