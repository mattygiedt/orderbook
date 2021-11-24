#include "gtest/gtest.h"
#include "orderbook/application_traits.h"

using namespace orderbook::data;

class OrderDataFixture : public ::testing::Test {
 private:
 public:
  static auto GreaterThanTest() -> void {
    LimitOrder order_a;
    LimitOrder order_b;

    order_a.SetOrderPrice(555);  // NOLINT
    order_b.SetOrderPrice(555);  // NOLINT
    ASSERT_FALSE(order_a > order_b);
    ASSERT_FALSE(order_b > order_a);

    order_b.SetOrderPrice(556);  // NOLINT
    ASSERT_FALSE(order_a > order_b);
    ASSERT_TRUE(order_b > order_a);

    order_b.SetOrderPrice(-555);  // NOLINT
    ASSERT_TRUE(order_a > order_b);
    ASSERT_FALSE(order_b > order_a);

    order_b.SetOrderPrice(-355);  // NOLINT
    ASSERT_TRUE(order_a > order_b);
    ASSERT_FALSE(order_b > order_a);
  }
};

TEST_F(OrderDataFixture, empty_test) { GreaterThanTest(); }  // NOLINT
