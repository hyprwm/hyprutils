#pragma once
#include <functional>
#include <vector>
#include <string>

namespace Hyprutils {
    namespace String {
        class CVarList2 {
          public:
            /** Split string into arg list
                Prefer this over CConstVarList / CVarList, this is better.

                @param lastArgNo stop splitting after argv reaches maximum size, last arg will contain rest of unsplit args
                @param delim if delimiter is 's', use std::isspace
                @param removeEmpty remove empty args from argv
                @param allowEscape whether to allow escaping the delimiter
            */
            CVarList2(std::string&& in, const size_t lastArgNo = 0, const char delim = ',', const bool removeEmpty = false, const bool allowEscape = true);

            ~CVarList2() = default;

            size_t size() const {
                return m_args.size();
            }

            std::string      join(const std::string& joiner, size_t from = 0, size_t to = 0) const;
            void             append(std::string&& arg);
            bool             contains(const std::string& el);

            std::string_view operator[](const size_t& idx) const {
                if (idx >= m_args.size())
                    return "";
                return m_args[idx];
            }

            // for range-based loops
            std::vector<std::string_view>::const_iterator begin() const {
                return m_args.begin();
            }
            std::vector<std::string_view>::const_iterator end() const {
                return m_args.end();
            }

          private:
            std::string                   m_inString;
            std::vector<std::string>      m_copyStrings;
            std::vector<std::string_view> m_args;
        };
    }
}
