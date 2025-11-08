#include "I18nEngine.hpp"

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

    if (!entry || !entry->exists) {
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