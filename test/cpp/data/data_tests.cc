#include "gtest/gtest.h"
#include "orderbook/application_traits.h"

using namespace orderbook::data;

class OrderDataFixture : public ::testing::Test {
 private:
  using Price = orderbook::data::Price;

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

  static auto DoubleConversionTest() -> void {
    double orig_prc = 1234.56789;  // NOLINT
    Price prc = orderbook::data::ToPrice(orig_prc);
    double convert_prc = orderbook::data::ToDouble(prc);
    ASSERT_TRUE(orig_prc == convert_prc);

    // spdlog::info("{}", orig_prc);
    // spdlog::info("{}", prc);
    // spdlog::info("{} * {}", prc,
    // orderbook::data::internal::kPriceToDoubleMult); spdlog::info("{}",
    // convert_prc);
  }
};

TEST_F(OrderDataFixture, greater_than_test) { GreaterThanTest(); }  // NOLINT
TEST_F(OrderDataFixture, double_conversion_test) {                  // NOLINT
  DoubleConversionTest();
}
