#pragma once
#include <string>

namespace Hyprutils {
    namespace String {
        // trims beginning and end of whitespace characters
        std::string trim(const std::string& in);
        bool        isNumber(const std::string& str, bool allowfloat = false);
        void        replaceInString(std::string& string, const std::string& what, const std::string& to);
    };
};