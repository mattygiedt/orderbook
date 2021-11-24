#include "gtest/gtest.h"
#include "orderbook/application_traits.h"

using namespace orderbook::data;

template <typename Traits>
class ContainerFixture : public ::testing::Test {
 private:
  using Book = typename Traits::BookType;
  using BidContainer = typename Traits::BidContainerType;
  using AskContainer = typename Traits::AskContainerType;
  using Order = typename Traits::OrderType;

  static constexpr auto kClientOrderIdSize = 8;

  // https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
  static auto RandomStringView(const std::size_t length) -> std::string_view {
    auto rand_char = []() -> char {
      const char charset[] =  // NOLINT
          "0123456789"
          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
          "abcdefghijklmnopqrstuvwxyz";
      const std::size_t max_index = (sizeof(charset) - 1);
      return charset[rand() % max_index];
    };

    std::string rand_str(length, 0);
    std::generate_n(rand_str.begin(), length, rand_char);
    std::string_view str_view = rand_str;
    return str_view;
  }

  static auto SetOrder(Order& order, ClientOrderId client_order_id, OrderId id,
                       Price price, Quantity quantity, Side side) -> void {
    order.SetClientOrderId(client_order_id)
        .SetOrderId(id)
        .SetOrderPrice(price)
        .SetOrderQuantity(quantity)
        .SetSide(side);
  }

  static auto MakeOrder(Price price, Quantity quantity, Side side) -> Order {
    static OrderId order_id{0};
    Order order;
    SetOrder(order, RandomStringView(kClientOrderIdSize), ++order_id, price,
             quantity, side);
    return order;
  }

  static auto MakeCancel(const Order& order, const OrderId id)
      -> OrderCancelRequest {
    OrderCancelRequest request;
    request.SetOrderId(id);
    request.SetSide(order.GetSide());
    request.SetOrderPrice(order.GetOrderPrice());
    request.SetOrderQuantity(order.GetOrderQuantity());
    request.SetClientOrderId(order.GetClientOrderId());
    return request;
  }

 public:
  static auto EmptyTest() -> void {
    BidContainer container;
    ASSERT_TRUE(container.IsEmpty());
  }

  static auto AddTest() -> void {
    BidContainer container;
    ASSERT_TRUE(container.IsEmpty());

    auto order = MakeOrder(456,  // NOLINT
                           33,   // NOLINT
                           SideCode::kBuy);

    auto&& [added, added_order] = container.Add(order);
    ASSERT_TRUE(added);
    ASSERT_TRUE(added_order.GetOrderId() == order.GetOrderId());
    ASSERT_FALSE(container.IsEmpty());

    auto& resting_order = container.Front();

    ASSERT_TRUE(resting_order.GetOrderQuantity() == order.GetOrderQuantity());
    ASSERT_TRUE(resting_order.GetOrderPrice() == order.GetOrderPrice());
    ASSERT_TRUE(resting_order.GetClientOrderId() == order.GetClientOrderId());
    ASSERT_TRUE(resting_order.GetSide() == order.GetSide());
  }

  static auto RemoveTest() -> void {
    BidContainer container;
    ASSERT_TRUE(container.IsEmpty());

    auto order = MakeOrder(456,  // NOLINT
                           33,   // NOLINT
                           SideCode::kBuy);

    auto&& [added, added_order] = container.Add(order);
    ASSERT_TRUE(added);
    ASSERT_FALSE(container.IsEmpty());

    auto cancel = MakeCancel(order, added_order.GetOrderId());

    auto&& [removed, removed_order] = container.Remove(cancel);

    ASSERT_TRUE(removed);
    ASSERT_TRUE(removed_order.GetOrderId() == added_order.GetOrderId());
  }

  static auto PriceLevelTest() -> void {
    BidContainer bids;
    AskContainer asks;
    std::pair<bool, Order> add_pair;

    add_pair = bids.Add(MakeOrder(20, 10, SideCode::kBuy));  // NOLINT
    ASSERT_TRUE(add_pair.first);

    add_pair = bids.Add(MakeOrder(19, 10, SideCode::kBuy));  // NOLINT
    ASSERT_TRUE(add_pair.first);

    add_pair = bids.Add(MakeOrder(21, 10, SideCode::kBuy));  // NOLINT
    ASSERT_TRUE(add_pair.first);

    add_pair = bids.Add(MakeOrder(20, 10, SideCode::kBuy));  // NOLINT
    ASSERT_TRUE(add_pair.first);

    add_pair = asks.Add(MakeOrder(20, 10, SideCode::kSell));  // NOLINT
    ASSERT_TRUE(add_pair.first);

    add_pair = asks.Add(MakeOrder(19, 10, SideCode::kSell));  // NOLINT
    ASSERT_TRUE(add_pair.first);

    add_pair = asks.Add(MakeOrder(21, 10, SideCode::kSell));  // NOLINT
    ASSERT_TRUE(add_pair.first);

    add_pair = asks.Add(MakeOrder(20, 10, SideCode::kSell));  // NOLINT
    ASSERT_TRUE(add_pair.first);

    ASSERT_TRUE(bids.Count() == 4);  // NOLINT
    ASSERT_TRUE(asks.Count() == 4);  // NOLINT

    ASSERT_TRUE(bids.Front().GetOrderPrice() == 21);  // NOLINT
    ASSERT_TRUE(asks.Front().GetOrderPrice() == 19);  // NOLINT

    // IMPORTANT! -- notice how we're using Order and not OrderCancelRequest
    bids.Remove(bids.Front());
    asks.Remove(asks.Front());

    ASSERT_TRUE(bids.Count() == 3);  // NOLINT
    ASSERT_TRUE(asks.Count() == 3);  // NOLINT

    ASSERT_TRUE(bids.Front().GetOrderPrice() == 20);  // NOLINT
    ASSERT_TRUE(asks.Front().GetOrderPrice() == 20);  // NOLINT
  }
};

using MapListContainerFixture =
    ContainerFixture<orderbook::MapListOrderBookTraits>;

TEST_F(MapListContainerFixture, empty_test) { EmptyTest(); }  // NOLINT

TEST_F(MapListContainerFixture, add_test) { AddTest(); }  // NOLINT

TEST_F(MapListContainerFixture, remove_test) { RemoveTest(); }  // NOLINT

TEST_F(MapListContainerFixture, price_level_test) {  // NOLINT
  PriceLevelTest();
}
