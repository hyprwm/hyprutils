
#include <i18n/I18nEngine.hpp>

#include <gtest/gtest.h>

using namespace Hyprutils::I18n;

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
    engine.registerEntry("ts_TST", TXT_KEY_FALLBACK, "Hello {var1} world {var2}");

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

    EXPECT_EQ(engine.localizeEntry("ts_TST", TXT_KEY_FALLBACK, {{"var1", "hi"}, {"var2", "!"}}), "Hello hi world !");
    // Order shouldn't matter
    EXPECT_EQ(engine.localizeEntry("ts_TST", TXT_KEY_FALLBACK, {{"var2", "!"}, {"var1", "hi"}}), "Hello hi world !");
}
