#pragma once

#include <hyprutils/i18n/I18nEngine.hpp>

namespace Hyprutils::I18n {
    struct SI18nTranslationEntry {
        bool          exists = false;
        std::string   entry  = "";
        translationFn fn     = nullptr;
    };

    struct SI18nEngineImpl {
        std::unordered_map<std::string, std::vector<SI18nTranslationEntry>> entries;
        std::string                                                         fallbackLocale = "en_US";
    };
};