#pragma once
#include <functional>
#include <vector>
#include <string>

namespace Hyprutils {
    namespace String {
        class CVarList {
          public:
            /** Split string into arg list
                * @param in The string to split
                * @param lastArgNo The number of arguments to split into
                * @param delim The delimiter to use for splitting
                * @param removeEmpty Whether to remove empty arguments
            */
            CVarList(const std::string& in, const size_t lastArgNo = 0, const char delim = ',', const bool removeEmpty = false);

            /** Split string into arg list with escape handling
                * @param in The string to split
                * @param lastArgNo The number of arguments to split into
                * @param delim The delimiter to use for splitting
                * @param removeEmpty Whether to remove empty arguments
                * @param handleEscape Whether to handle escape characters
            */
            CVarList(const std::string& in, const size_t lastArgNo, const char delim, const bool removeEmpty, const bool handleEscape);

            ~CVarList() = default;

            size_t size() const {
                return m_vArgs.size();
            }

            std::string join(const std::string& joiner, size_t from = 0, size_t to = 0) const;

            void        map(std::function<void(std::string&)> func) {
                for (auto& s : m_vArgs)
                    func(s);
            }

            void append(const std::string arg) {
                m_vArgs.emplace_back(arg);
            }

            std::string operator[](const size_t& idx) const {
                if (idx >= m_vArgs.size())
                    return "";
                return m_vArgs[idx];
            }

            // for range-based loops
            std::vector<std::string>::iterator begin() {
                return m_vArgs.begin();
            }
            std::vector<std::string>::const_iterator begin() const {
                return m_vArgs.begin();
            }
            std::vector<std::string>::iterator end() {
                return m_vArgs.end();
            }
            std::vector<std::string>::const_iterator end() const {
                return m_vArgs.end();
            }

            bool contains(const std::string& el) {
                for (auto& a : m_vArgs) {
                    if (a == el)
                        return true;
                }

                return false;
            }

          private:
            std::vector<std::string> m_vArgs;
        };
    }
}
