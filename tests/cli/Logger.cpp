#include <cli/Logger.hpp>
#include <hyprutils/os/File.hpp>

#include <gtest/gtest.h>

#include <filesystem>

using namespace Hyprutils::CLI;
using namespace Hyprutils;

TEST(CLI, Logger) {
    CLogger logger;
    logger.log(Hyprutils::CLI::LOG_DEBUG, "Hello!");

    EXPECT_EQ(logger.rollingLog(), "DEBUG ]: Hello!");

    logger.log(Hyprutils::CLI::LOG_TRACE, "Hello!");

    EXPECT_EQ(logger.rollingLog(), "DEBUG ]: Hello!");

    logger.setTrace(true);

    logger.log(Hyprutils::CLI::LOG_TRACE, "Hello, {}!", "Trace");

    EXPECT_EQ(logger.rollingLog(), "DEBUG ]: Hello!\nTRACE ]: Hello, Trace!");

    logger.setEnableStdout(false);

    logger.log(Hyprutils::CLI::LOG_ERR, "Error");

    EXPECT_EQ(logger.rollingLog(), "DEBUG ]: Hello!\nTRACE ]: Hello, Trace!");

    auto res = logger.setOutputFile("./loggerFile.log");
    EXPECT_TRUE(res);

    logger.log(LOG_DEBUG, "Hi file!");

    res = logger.setOutputFile(""); // clear
    EXPECT_TRUE(res);

    auto fileRead = File::readFileAsString("./loggerFile.log");
    EXPECT_TRUE(fileRead);

    EXPECT_EQ(fileRead.value_or(""), "DEBUG ]: Hi file!\n");

    std::error_code ec;
    std::filesystem::remove("./loggerFile.log", ec);

    // TODO: maybe find a way to test the times and color?
    logger.setEnableStdout(true);
    logger.setTime(true);

    logger.log(Hyprutils::CLI::LOG_WARN, "Timed warning!");

    logger.setEnableColor(false);

    logger.log(Hyprutils::CLI::LOG_CRIT, "rip");

    // spam some logs to check rolling
    for (size_t i = 0; i < 1000; ++i) {
        logger.log(LOG_DEBUG, "Log log log!");
    }

    EXPECT_TRUE(logger.rollingLog().size() < 4096);
    EXPECT_TRUE(logger.rollingLog().starts_with("DEBUG")); // test the breaking is done correctly
}