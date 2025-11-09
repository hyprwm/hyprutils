#include <hyprutils/i18n/I18nEngine.hpp>

using namespace Hyprutils::I18n;

static std::string extractLocale(std::string locale) {
    // localeStr is very arbitrary... from my testing, it can be:
    // en_US.UTF-8
    // LC_CTYPE=en_US
    // POSIX
    // *
    //
    // We only return e.g. en_US or pl_PL, or pl

    if (locale == "POSIX")
        return "en_US";
    if (locale == "*")
        return "en_US";

    if (locale.contains('='))
        locale = locale.substr(locale.find('=') + 1);

    if (locale.contains('.'))
        locale = locale.substr(0, locale.find('.'));

    return locale;
}

CI18nLocale::CI18nLocale(std::string fullLocale) : m_rawFullLocale(std::move(fullLocale)) {
    m_locale = extractLocale(m_rawFullLocale);
}

std::string CI18nLocale::locale() {
    return m_locale;
}

std::string CI18nLocale::stem() {
    if (m_locale.contains('_'))
        return m_locale.substr(0, m_locale.find('_'));
    return m_locale;
}

std::string CI18nLocale::full() {
    return m_rawFullLocale;
}

#ifdef HU_UNIT_TESTS

#include <gtest/gtest.h>
#include <hyprutils/i18n/I18nEngine.hpp>

TEST(I18n, Locale) {
    CI18nEngine engine;

    EXPECT_EQ(extractLocale("pl_PL.UTF-8"), "pl_PL");
    EXPECT_EQ(extractLocale("POSIX"), "en_US");
    EXPECT_EQ(extractLocale("*"), "en_US");
    EXPECT_EQ(extractLocale("LC_CTYPE=pl_PL.UTF-8"), "pl_PL");
}

#endif
