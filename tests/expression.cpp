#include <hyprutils/string/Expression.hpp>
#include "shared.hpp"

using namespace Hyprutils::Expression;

int main() {
    int                                     ret = 0;

    std::unordered_map<std::string, int>    VARS_INT    = {{"x", 5}, {"y", 10}};
    std::unordered_map<std::string, float>  VARS_FLOAT  = {{"x", 5.0f}, {"y", 10.0f}};
    std::unordered_map<std::string, double> VARS_DOUBLE = {{"x", 5.0f}, {"y", 10.0f}};
    try {
        // int
        EXPECT_RESULT_PASS(eval<int>("(2+3)*4", VARS_INT), 20);
        EXPECT_RESULT_PASS(eval<int>("x + y", VARS_INT), 15);
        EXPECT_RESULT_PASS(eval<int>("x - y", VARS_INT), -5);
        EXPECT_RESULT_PASS(eval<int>("x * y", VARS_INT), 50);
        EXPECT_RESULT_PASS(eval<int>("y / x", VARS_INT), 2);

        EXPECT_RESULT_FAIL(eval<int>("y / 0", VARS_INT), "Division by zero");
        EXPECT_RESULT_FAIL(eval<int>("unknownVar + 1", VARS_INT), "Unknown variable: unknownVar");

        // float
        EXPECT_RESULT_PASS(eval<float>("(2.0+3.0)*4.0", VARS_FLOAT), 20.0f);
        EXPECT_RESULT_PASS(eval<float>("x + y", VARS_FLOAT), 15.0f);
        EXPECT_RESULT_PASS(eval<float>("x - y", VARS_FLOAT), -5.0f);
        EXPECT_RESULT_PASS(eval<float>("x * y", VARS_FLOAT), 50.0f);
        EXPECT_RESULT_PASS(eval<float>("y / x", VARS_FLOAT), 2.0f);

        EXPECT_RESULT_FAIL(eval<float>("y / 0.0", VARS_FLOAT), "Division by zero");
        EXPECT_RESULT_FAIL(eval<float>("unknownVar + 1", VARS_FLOAT), "Unknown variable: unknownVar");

        // double
        EXPECT_RESULT_PASS(eval<double>("(2.0 + 3.0) * 4.0", VARS_DOUBLE), 20.0);
        EXPECT_RESULT_PASS(eval<double>("x + y", VARS_DOUBLE), 15.0);
        EXPECT_RESULT_PASS(eval<double>("x - y", VARS_DOUBLE), -5.0);
        EXPECT_RESULT_PASS(eval<double>("x * y", VARS_DOUBLE), 50.0);
        EXPECT_RESULT_PASS(eval<double>("y / x", VARS_DOUBLE), 2.0);

        EXPECT_RESULT_FAIL(eval<double>("y / 0.0", VARS_DOUBLE), "Division by zero");
        EXPECT_RESULT_FAIL(eval<double>("unknownVar + 1", VARS_DOUBLE), "Unknown variable: unknownVar");
    } catch (const std::exception& e) {
        std::cout << e.what() << "\n";
        ret = 1;
    }

    return ret;
}
