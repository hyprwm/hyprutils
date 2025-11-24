#include <cli/ArgumentParser.hpp>

#include <gtest/gtest.h>

#include <print>

using namespace Hyprutils::CLI;
using namespace Hyprutils;

constexpr const char* DESC_TEST = R"#(┏ My description
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┣ --hello                  -h          | Says hello                              ┃
┣ --hello2                 -e          | Says hello 2                            ┃
┣ --value                  -v [float]  | Sets a valueeeeeee                      ┃
┣ --longlonglonglongintopt -l [int]    | Long long                               ┃
┣                                        maaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa┃
┣                                        aaaaaaaaaaan maaan man maaan man maaan  ┃
┣                                        man maaan man                           ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
)#";

TEST(CLI, ArgumentParser) {
    std::vector<const char*> argv = {"app", "--hello", "--value", "0.2"};

    CArgumentParser          parser(argv);

    EXPECT_TRUE(parser.registerBoolOption("hello", "h", "Says hello"));
    EXPECT_TRUE(parser.registerBoolOption("hello2", "e", "Says hello 2"));
    EXPECT_TRUE(parser.registerFloatOption("value", "v", "Sets a valueeeeeee"));
    EXPECT_TRUE(parser.registerIntOption("longlonglonglongintopt", "l", "Long long maaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaan maaan man maaan man maaan man maaan man"));

    auto result = parser.parse();

    EXPECT_TRUE(result.has_value());

    std::println("{}", parser.getDescription("My description"));

    if (!result.has_value())
        std::println("Error: {}", result.error());

    EXPECT_EQ(parser.getBool("hello").value_or(false), true);
    EXPECT_EQ(parser.getBool("hello2").value_or(false), false);
    EXPECT_EQ(parser.getFloat("value").value_or(0.F), 0.2F);

    EXPECT_EQ(parser.getDescription("My description"), DESC_TEST);

    CArgumentParser parser2(argv);

    EXPECT_TRUE(parser2.registerBoolOption("hello2", "e", "Says hello 2"));
    EXPECT_TRUE(parser2.registerFloatOption("value", "v", "Sets a valueeeeeee"));
    EXPECT_TRUE(parser2.registerIntOption("longlonglonglongintopt", "l", "Long long maaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaan maaan man maaan man maaan man maaan man"));
    EXPECT_TRUE(parser2.registerFloatOption("value2", "", "Sets a valueeeeeee 2"));
    EXPECT_TRUE(!parser2.registerFloatOption("", "a", "Sets a valueeeeeee 2"));

    auto result2 = parser2.parse();

    EXPECT_TRUE(!result2.has_value());

    std::vector<const char*> argv3 = {"app", "--hello", "--value"};

    CArgumentParser          parser3(argv3);

    EXPECT_TRUE(parser3.registerBoolOption("hello2", "e", "Says hello 2"));
    EXPECT_TRUE(parser3.registerFloatOption("value", "v", "Sets a valueeeeeee"));
    EXPECT_TRUE(parser3.registerIntOption("longlonglonglongintopt", "l", "Long long maaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaan maaan man maaan man maaan man maaan man"));

    auto result3 = parser3.parse();

    EXPECT_TRUE(!result3.has_value());

    std::vector<const char*> argv4 = {"app", "--value", "hi", "-w", "2"};

    CArgumentParser          parser4(argv4);

    EXPECT_TRUE(parser4.registerStringOption("value", "v", "Sets a valueeeeeee"));
    EXPECT_TRUE(parser4.registerIntOption("value2", "w", "Sets a valueeeeeee 2"));

    auto result4 = parser4.parse();

    EXPECT_TRUE(result4.has_value());

    EXPECT_EQ(parser4.getString("value").value_or(""), "hi");
    EXPECT_EQ(parser4.getInt("value2").value_or(0), 2);

    std::vector<const char*> argv5 = {
        "app",
        "e",
    };

    CArgumentParser parser5(argv5);

    EXPECT_TRUE(parser5.registerStringOption("value", "v", "Sets a valueeeeeee"));
    EXPECT_TRUE(parser5.registerStringOption("value2", "w", "Sets a valueeeeeee 2"));

    auto result5 = parser5.parse();

    EXPECT_TRUE(!result5.has_value());

    CArgumentParser parser6(argv5);

    EXPECT_TRUE(parser6.registerStringOption("aa", "v", "Sets a valueeeeeee"));
    EXPECT_TRUE(!parser6.registerStringOption("aa", "w", "Sets a valueeeeeee 2"));
    EXPECT_TRUE(parser6.registerStringOption("bb", "b", "Sets a valueeeeeee"));
    EXPECT_TRUE(!parser6.registerStringOption("cc", "b", "Sets a valueeeeeee 2"));
}