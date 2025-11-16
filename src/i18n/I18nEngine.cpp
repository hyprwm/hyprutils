#include "I18nEngine.hpp"

#include <algorithm>
#include <format>
#include <locale>
#include <hyprutils/utils/ScopeGuard.hpp>

using namespace Hyprutils::I18n;
using namespace Hyprutils;
using namespace Hyprutils::Utils;

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

            const auto stem    = locale.substr(0, locale.find('_') + 1);
            const auto stemRaw = locale.substr(0, locale.find('_'));
            for (const auto& [k, v] : m_impl->entries) {
                if (k.starts_with(stem) || k == stemRaw) {
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

    std::string_view rawStr = entry->entry;
    std::string      fnStringContainer;

    if (entry->fn) {
        fnStringContainer = entry->fn(map);
        rawStr            = fnStringContainer;
    }

    struct SRange {
        size_t             begin = 0;
        size_t             end   = 0;
        const std::string* val   = nullptr;
    };
    std::vector<SRange> rangesFound;

    // discover all replacable ranges
    for (const auto& [k, v] : map) {
        size_t start = rawStr.find(k, 0);
        while (start != std::string::npos) {
            if (start == 0 || start + k.size() >= rawStr.size())
                break;

            // always move the pointer
            CScopeGuard x([&start, &rawStr, &k] { start = rawStr.find(k, start + 1); });

            if (rawStr[start - 1] != '{' || rawStr[start + k.size()] != '}')
                continue;

            // add range
            rangesFound.emplace_back(SRange{.begin = start - 1, .end = start + k.size() + 1, .val = &v});
        }
    }

    if (rangesFound.empty())
        return std::string{rawStr};

    // build the new string. First, sort our entries
    std::ranges::sort(rangesFound, [](const auto& a, const auto& b) { return a.begin - b.begin; });

    // calc the size
    size_t stringLen = 0;
    size_t lastBegin = 0;
    for (const auto& r : rangesFound) {
        stringLen += r.begin - lastBegin + r.val->size();
        lastBegin = r.end;
    }
    stringLen += rawStr.size() - lastBegin;

    lastBegin                = 0;
    const auto  ORIGINAL_STR = std::string_view{rawStr};
    std::string newStr;
    newStr.reserve(stringLen);

    for (const auto& r : rangesFound) {
        newStr += ORIGINAL_STR.substr(lastBegin, r.begin);
        newStr += *r.val;

        lastBegin = r.end;
    }
    newStr += ORIGINAL_STR.substr(lastBegin);

    return newStr;
}

CI18nLocale CI18nEngine::getSystemLocale() {
    try {
        return CI18nLocale(std::locale("").name());
    } catch (...) { return CI18nLocale("en_US.UTF-8"); }
}

#ifdef HU_UNIT_TESTS

#include <gtest/gtest.h>

enum eTxtKeys : uint64_t {
    TXT_KEY_HELLO,
    TXT_KEY_I_HAVE_APPLES,
    TXT_KEY_FALLBACK,
};

TEST(I18n, Engine) {
    CI18nEngine engine;

    engine.setFallbackLocale("en_US");

    engine.registerEntry("en_US", TXT_KEY_HELLO, "Hello World!");
    engine.registerEntry("en_US", TXT_KEY_I_HAVE_APPLES, [](const translationVarMap& m) {
        if (std::stoi(m.at("count")) == 1)
            return "I have {count} apple.";
        else
            return "I have {count} apples.";
    });
    engine.registerEntry("en_US", TXT_KEY_FALLBACK, "Fallback string!");

    engine.registerEntry("pl_PL", TXT_KEY_HELLO, "Witaj świecie!");
    engine.registerEntry("pl_PL", TXT_KEY_I_HAVE_APPLES, [](const translationVarMap& m) {
        const auto COUNT = std::stoi(m.at("count"));
        if (COUNT == 1)
            return "Mam {count} jabłko.";
        else if (COUNT < 5)
            return "Mam {count} jabłka.";
        else
            return "Mam {count} jabłek.";
    });

    engine.registerEntry("es_XX", TXT_KEY_FALLBACK, "I don't speak spanish");
    engine.registerEntry("es_ES", TXT_KEY_FALLBACK, "I don't speak spanish here either");

    engine.registerEntry("am", TXT_KEY_FALLBACK, "Amongus!");

    EXPECT_EQ(engine.localizeEntry("en_US", TXT_KEY_HELLO, {}), "Hello World!");
    EXPECT_EQ(engine.localizeEntry("pl_PL", TXT_KEY_HELLO, {}), "Witaj świecie!");
    EXPECT_EQ(engine.localizeEntry("de_DE", TXT_KEY_HELLO, {}), "Hello World!");

    EXPECT_EQ(engine.localizeEntry("en_US", TXT_KEY_I_HAVE_APPLES, {{"count", "1"}}), "I have 1 apple.");
    EXPECT_EQ(engine.localizeEntry("en_US", TXT_KEY_I_HAVE_APPLES, {{"count", "2"}}), "I have 2 apples.");

    EXPECT_EQ(engine.localizeEntry("pl_PL", TXT_KEY_I_HAVE_APPLES, {{"count", "1"}}), "Mam 1 jabłko.");
    EXPECT_EQ(engine.localizeEntry("pl_PL", TXT_KEY_I_HAVE_APPLES, {{"count", "2"}}), "Mam 2 jabłka.");
    EXPECT_EQ(engine.localizeEntry("pl_PL", TXT_KEY_I_HAVE_APPLES, {{"count", "5"}}), "Mam 5 jabłek.");

    EXPECT_EQ(engine.localizeEntry("pl", TXT_KEY_I_HAVE_APPLES, {{"count", "5"}}), "Mam 5 jabłek.");

    EXPECT_EQ(engine.localizeEntry("pl_XX", TXT_KEY_I_HAVE_APPLES, {{"count", "5"}}), "Mam 5 jabłek.");
    EXPECT_EQ(engine.localizeEntry("en_XX", TXT_KEY_I_HAVE_APPLES, {{"count", "2"}}), "I have 2 apples.");

    EXPECT_EQ(engine.localizeEntry("es_YY", TXT_KEY_FALLBACK, {}), "I don't speak spanish here either");
    EXPECT_EQ(engine.localizeEntry("es_XX", TXT_KEY_FALLBACK, {}), "I don't speak spanish");

    EXPECT_EQ(engine.localizeEntry("pl_PL", TXT_KEY_FALLBACK, {}), "Fallback string!");

    EXPECT_EQ(engine.localizeEntry("am_AM", TXT_KEY_FALLBACK, {}), "Amongus!");

    // test weird translations
    engine.registerEntry("ts", TXT_KEY_HELLO, "count}");
    EXPECT_EQ(engine.localizeEntry("ts", TXT_KEY_HELLO, {{"count", "1"}}), "count}");

    engine.registerEntry("ts", TXT_KEY_HELLO, "{count");
    EXPECT_EQ(engine.localizeEntry("ts", TXT_KEY_HELLO, {{"count", "1"}}), "{count");

    EXPECT_EQ(engine.localizeEntry("ts", 42069 /* invalid key */, {{"count", "1"}}), "");
}

#endif
