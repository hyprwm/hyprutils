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
}