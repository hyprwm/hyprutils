#pragma once
#include <string>

namespace Hyprutils {
    namespace String {
        // trims beginning and end of whitespace characters
        std::string      trim(const char* in);
        std::string      trim(const std::string& in);
        std::string_view trim(const std::string_view& in);
        bool             isNumber(const std::string& str, bool allowfloat = false);
        void             replaceInString(std::string& string, const std::string& what, const std::string& to);
        bool             truthy(const std::string_view& in);
    };
};