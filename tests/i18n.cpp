#include <hyprutils/i18n/I18nEngine.hpp>
#include "shared.hpp"
#include <print>

using namespace Hyprutils::I18n;

enum eTxtKeys : uint64_t {
    TXT_KEY_HELLO,
    TXT_KEY_I_HAVE_APPLES,
    TXT_KEY_FALLBACK,
};

int main(int argc, char** argv, char** envp) {
    int         ret = 0;

    CI18nEngine engine;

    std::println("System locale: {}, stem: {}", engine.getSystemLocale().locale(), engine.getSystemLocale().stem());

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

    EXPECT(engine.localizeEntry("en_US", TXT_KEY_HELLO, {}), "Hello World!");
    EXPECT(engine.localizeEntry("pl_PL", TXT_KEY_HELLO, {}), "Witaj świecie!");
    EXPECT(engine.localizeEntry("de_DE", TXT_KEY_HELLO, {}), "Hello World!");

    EXPECT(engine.localizeEntry("en_US", TXT_KEY_I_HAVE_APPLES, {{"count", "1"}}), "I have 1 apple.");
    EXPECT(engine.localizeEntry("en_US", TXT_KEY_I_HAVE_APPLES, {{"count", "2"}}), "I have 2 apples.");

    EXPECT(engine.localizeEntry("pl_PL", TXT_KEY_I_HAVE_APPLES, {{"count", "1"}}), "Mam 1 jabłko.");
    EXPECT(engine.localizeEntry("pl_PL", TXT_KEY_I_HAVE_APPLES, {{"count", "2"}}), "Mam 2 jabłka.");
    EXPECT(engine.localizeEntry("pl_PL", TXT_KEY_I_HAVE_APPLES, {{"count", "5"}}), "Mam 5 jabłek.");

    EXPECT(engine.localizeEntry("pl", TXT_KEY_I_HAVE_APPLES, {{"count", "5"}}), "Mam 5 jabłek.");

    EXPECT(engine.localizeEntry("pl_XX", TXT_KEY_I_HAVE_APPLES, {{"count", "5"}}), "Mam 5 jabłek.");
    EXPECT(engine.localizeEntry("en_XX", TXT_KEY_I_HAVE_APPLES, {{"count", "2"}}), "I have 2 apples.");

    EXPECT(engine.localizeEntry("es_YY", TXT_KEY_FALLBACK, {}), "I don't speak spanish here either");
    EXPECT(engine.localizeEntry("es_XX", TXT_KEY_FALLBACK, {}), "I don't speak spanish");

    EXPECT(engine.localizeEntry("pl_PL", TXT_KEY_FALLBACK, {}), "Fallback string!");

    return ret;
}