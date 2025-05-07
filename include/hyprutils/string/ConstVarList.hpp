#pragma once
#include <functional>
#include <vector>
#include <string>

namespace Hyprutils {
    namespace String {
        class CConstVarList {
          public:
            /** Split string into an immutable arg list
                @param lastArgNo stop splitting after argv reaches maximum size, last arg will contain rest of unsplit args
                @param delim if delimiter is 's', use std::isspace
                @param removeEmpty remove empty args from argv
            */
            CConstVarList(const std::string& in, const size_t lastArgNo = 0, const char delim = ',', const bool removeEmpty = false);

            ~CConstVarList() = default;

            size_t size() const {
                return m_args.size();
            }

            std::string join(const std::string& joiner, size_t from = 0, size_t to = 0) const;

            void        map(std::function<void(const std::string_view&)> func) {
                for (auto& s : m_args)
                    func(s);
            }

            std::string_view operator[](const size_t& idx) const {
                if (idx >= m_args.size())
                    return "";
                return m_args[idx];
            }

            // for range-based loops
            std::vector<std::string_view>::iterator begin() {
                return m_args.begin();
            }
            std::vector<std::string_view>::const_iterator begin() const {
                return m_args.begin();
            }
            std::vector<std::string_view>::iterator end() {
                return m_args.end();
            }
            std::vector<std::string_view>::const_iterator end() const {
                return m_args.end();
            }

            bool contains(const std::string_view& el) {
                for (auto& a : m_args) {
                    if (a == el)
                        return true;
                }

                return false;
            }

          private:
            std::string                   m_str;
            std::vector<std::string_view> m_args;
        };
    }
}
