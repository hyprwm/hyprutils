#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>

#include <gtest/gtest.h>
#include <utility>

using namespace Hyprutils::Memory;

namespace {
    class IBase {
      public:
        virtual ~IBase() = default;
        int value        = 0;
    };

    class CDerived : public IBase {};
}

TEST(Memory, weakPointerMoveAssignment) {
    auto lhsOwner = makeShared<int>(1);
    auto rhsOwner = makeShared<int>(2);

    CWeakPointer<int> lhs = lhsOwner;
    CWeakPointer<int> rhs = rhsOwner;

    auto* lhsImpl = lhs.impl_;
    auto* rhsImpl = rhs.impl_;

    ASSERT_EQ(lhsImpl->wref(), 1U);
    ASSERT_EQ(rhsImpl->wref(), 1U);

    lhs = std::move(rhs);

    EXPECT_EQ(lhs.impl_, rhsImpl);
    EXPECT_EQ(rhs.impl_, lhsImpl);
    EXPECT_EQ(lhsImpl->wref(), 1U);
    EXPECT_EQ(rhsImpl->wref(), 1U);
    EXPECT_EQ(*lhs, 2);
    EXPECT_EQ(*rhs, 1);
}

TEST(Memory, weakPointerConvertingMoveAssignment) {
    auto lhsOwner   = makeShared<CDerived>();
    auto rhsOwner   = makeShared<CDerived>();
    lhsOwner->value = 1;
    rhsOwner->value = 2;

    CWeakPointer<IBase>    lhs = lhsOwner;
    CWeakPointer<CDerived> rhs = rhsOwner;

    auto* lhsImpl = lhs.impl_;
    auto* rhsImpl = rhs.impl_;

    ASSERT_EQ(lhsImpl->wref(), 1U);
    ASSERT_EQ(rhsImpl->wref(), 1U);

    lhs = std::move(rhs);

    EXPECT_EQ(lhs.impl_, rhsImpl);
    EXPECT_EQ(rhs.impl_, lhsImpl);
    EXPECT_EQ(lhsImpl->wref(), 1U);
    EXPECT_EQ(rhsImpl->wref(), 1U);
    EXPECT_EQ(lhs->value, 2);
    EXPECT_EQ(rhs->value, 1);
}
