#pragma once

#include "../memory/UniquePtr.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>

namespace Hyprutils::I18n {
    struct SI18nEngineImpl;

    typedef std::unordered_map<std::string, std::string>         translationVarMap;
    typedef std::function<std::string(const translationVarMap&)> translationFn;

    class CI18nLocale {
      public:
        ~CI18nLocale() = default;

        std::string locale();
        std::string stem();
        std::string full();

      private:
        CI18nLocale(std::string fullLocale);

        std::string m_locale, m_rawFullLocale;

        friend class CI18nEngine;
    };

    class CI18nEngine {
      public:
        CI18nEngine();
        ~CI18nEngine();
        /*
            Register a translation entry. The internal translation db is kept as a vector,
            so make sure your keys are linear, don't use e.g. 2 billion as that will call
            .resize() on the vec to 2 billion.

            If you pass a Fn, you can do logic, e.g. "1 point" vs "2 points".
        */
        void        registerEntry(const std::string& locale, uint64_t key, std::string&& translationUTF8);
        void        registerEntry(const std::string& locale, uint64_t key, translationFn&& translationFn);

        void        setFallbackLocale(const std::string& locale);

        std::string localizeEntry(const std::string& locale, uint64_t key, const translationVarMap& map);

        CI18nLocale getSystemLocale();

      private:
        Memory::CUniquePointer<SI18nEngineImpl> m_impl;
    };
}