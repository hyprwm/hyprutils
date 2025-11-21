#include "I18nEngine.hpp"

using namespace Hyprutils::I18n;

std::string Hyprutils::I18n::extractLocale(std::string locale) {
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
