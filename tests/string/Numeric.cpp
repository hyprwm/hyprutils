#include <hyprutils/string/Numeric.hpp>

#include <gtest/gtest.h>
#include <cstdint>
#include <limits>

using namespace Hyprutils::String;

TEST(Numeric, intSuccess) {
    auto r = strToNumber<int>("42");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 42);

    auto neg = strToNumber<int>("-7");
    ASSERT_TRUE(neg.has_value());
    EXPECT_EQ(*neg, -7);

    auto zero = strToNumber<int>("0");
    ASSERT_TRUE(zero.has_value());
    EXPECT_EQ(*zero, 0);
}

TEST(Numeric, intBounds) {
    const auto maxStr = std::to_string(std::numeric_limits<int32_t>::max());
    auto       rMax   = strToNumber<int32_t>(maxStr);
    ASSERT_TRUE(rMax.has_value());
    EXPECT_EQ(*rMax, std::numeric_limits<int32_t>::max());

    const auto minStr = std::to_string(std::numeric_limits<int32_t>::min());
    auto       rMin   = strToNumber<int32_t>(minStr);
    ASSERT_TRUE(rMin.has_value());
    EXPECT_EQ(*rMin, std::numeric_limits<int32_t>::min());
}

TEST(Numeric, outOfRange) {
    auto r = strToNumber<int8_t>("999");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), NUMERIC_PARSE_OUT_OF_RANGE);

    // from_chars rejects '-' for unsigned types as invalid argument, not out-of-range
    auto r2 = strToNumber<uint8_t>("-1");
    ASSERT_FALSE(r2.has_value());
    EXPECT_EQ(r2.error(), NUMERIC_PARSE_BAD);
}

TEST(Numeric, garbage) {
    auto r = strToNumber<int>("12abc");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), NUMERIC_PARSE_GARBAGE);

    auto r2 = strToNumber<double>("1.0xyz");
    ASSERT_FALSE(r2.has_value());
    EXPECT_EQ(r2.error(), NUMERIC_PARSE_GARBAGE);
}

TEST(Numeric, bad) {
    auto r = strToNumber<int>("");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), NUMERIC_PARSE_BAD);

    auto r2 = strToNumber<int>("abc");
    ASSERT_FALSE(r2.has_value());
    EXPECT_EQ(r2.error(), NUMERIC_PARSE_BAD);

    auto r3 = strToNumber<int>("--1");
    ASSERT_FALSE(r3.has_value());
    EXPECT_EQ(r3.error(), NUMERIC_PARSE_BAD);
}

TEST(Numeric, floatSuccess) {
    auto r = strToNumber<double>("3.14");
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(*r, 3.14);

    auto neg = strToNumber<float>("-0.5");
    ASSERT_TRUE(neg.has_value());
    EXPECT_FLOAT_EQ(*neg, -0.5f);

    auto whole = strToNumber<double>("100");
    ASSERT_TRUE(whole.has_value());
    EXPECT_DOUBLE_EQ(*whole, 100.0);
}

TEST(Numeric, unsignedTypes) {
    auto r = strToNumber<uint64_t>("18446744073709551615");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, std::numeric_limits<uint64_t>::max());

    auto r2 = strToNumber<uint32_t>("0");
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(*r2, 0u);
}
