#include "I18nEngine.hpp"

#include <algorithm>
#include <format>
#include <locale>
#include <hyprutils/string/String.hpp>

using namespace Hyprutils::I18n;
using namespace Hyprutils;
using namespace Hyprutils::String;

CI18nEngine::CI18nEngine() : m_impl(Memory::makeUnique<SI18nEngineImpl>()) {
    ;
}
CI18nEngine::~CI18nEngine() = default;

void CI18nEngine::registerEntry(const std::string& locale, uint64_t key, std::string&& translationUTF8) {
    auto& entryVec = m_impl->entries[locale];

    if (entryVec.size() <= key)
        entryVec.resize(key + 1);

    entryVec[key].entry  = std::move(translationUTF8);
    entryVec[key].exists = true;
}

void CI18nEngine::registerEntry(const std::string& locale, uint64_t key, translationFn&& translationFn) {
    auto& entryVec = m_impl->entries[locale];

    if (entryVec.size() <= key)
        entryVec.resize(key + 1);

    entryVec[key].fn     = std::move(translationFn);
    entryVec[key].exists = true;
}

void CI18nEngine::setFallbackLocale(const std::string& locale) {
    m_impl->fallbackLocale = locale;
}

std::string CI18nEngine::localizeEntry(const std::string& locale, uint64_t key, const translationVarMap& map) {
    SI18nTranslationEntry* entry = nullptr;

    if (m_impl->entries.contains(locale) && m_impl->entries[locale].size() > key)
        entry = &m_impl->entries[locale][key];

    if (locale.contains('_')) {

        if (!entry || !entry->exists) {
            // try to fall back to lang_LANG
            auto stem      = locale.substr(0, locale.find('_'));
            auto stemUpper = stem;
            std::ranges::transform(stemUpper, stemUpper.begin(), ::toupper);
            auto newLocale = std::format("{}_{}", stem, stemUpper);
            if (m_impl->entries.contains(newLocale) && m_impl->entries[newLocale].size() > key)
                entry = &m_impl->entries[newLocale][key];
        }

        if (!entry || !entry->exists) {
            // try to fall back to any lang prefixed with our prefix

            auto stem = locale.substr(0, locale.find('_') + 1);
            for (const auto& [k, v] : m_impl->entries) {
                if (k.starts_with(stem)) {
                    if (m_impl->entries.contains(k) && m_impl->entries[k].size() > key)
                        entry = &m_impl->entries[k][key];

                    if (entry && entry->exists)
                        break;
                }
            }
        }
    } else {
        // locale doesn't have a _, e.g. pl
        // find any locale that has the same stem
        for (const auto& [k, v] : m_impl->entries) {
            if (k.starts_with(locale + "_") || k == locale) {
                if (m_impl->entries.contains(k) && m_impl->entries[k].size() > key)
                    entry = &m_impl->entries[k][key];

                if (entry && entry->exists)
                    break;
            }
        }
    }

    if (!entry || !entry->exists) {
        // fall back to general fallback
        if (m_impl->entries.contains(m_impl->fallbackLocale) && m_impl->entries[m_impl->fallbackLocale].size() > key)
            entry = &m_impl->entries[m_impl->fallbackLocale][key];
    }

    if (!entry || !entry->exists)
        return "";

    std::string rawStr = entry->entry;

    if (entry->fn)
        rawStr = entry->fn(map);

    for (const auto& e : map) {
        replaceInString(rawStr, "{" + e.first + "}", e.second);
    }

    return rawStr;
}

std::string CI18nEngine::getSystemLocale() {
    std::locale locale("");
    auto        localeStr = locale.name();

    // localeStr is very arbitrary... from my testing, it can be:
    // en_US.UTF-8
    // LC_CTYPE=en_US
    // POSIX
    // *
    //
    // We only return e.g. en_US or pl_PL, or pl

    if (localeStr == "POSIX")
        return "en_US";
    if (localeStr == "*")
        return "en_US";

    if (localeStr.contains('='))
        localeStr = localeStr.substr(localeStr.find('=') + 1);

    if (localeStr.contains('.'))
        localeStr = localeStr.substr(0, localeStr.find('.'));

    return localeStr;
}
