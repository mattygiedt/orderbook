#include "gtest/gtest.h"
#include "orderbook/application_traits.h"

using namespace orderbook::data;

// clang-format off
template <typename Traits>

  requires( orderbook::container::ContainerConcept<typename Traits::BidContainerType,
                                                   typename Traits::OrderType> &&
            orderbook::container::ContainerConcept<typename Traits::AskContainerType,
                                                   typename Traits::OrderType> )

class ContainerFixture : public ::testing::Test {  // clang-format on
 private:
  using Book = typename Traits::BookType;
  using BidContainer = typename Traits::BidContainerType;
  using AskContainer = typename Traits::AskContainerType;
  using Order = typename Traits::OrderType;
  using OrderPtr = boost::intrusive_ptr<Order>;
  using Pool = typename Traits::PoolType;
  using NewOrderSingle = orderbook::data::NewOrderSingle;

  inline static OrderId order_id{0};
  inline static Pool& pool = Pool::Instance();
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

  static auto MakeOrder(const NewOrderSingle& new_order_single) -> OrderPtr {
    auto ord = pool.MakeIntrusive();
    ord->SetOrderId(++order_id)
        .SetRoutingId(new_order_single.GetRoutingId())
        .SetSessionId(new_order_single.GetSessionId())
        .SetAccountId(new_order_single.GetAccountId())
        .SetInstrumentId(new_order_single.GetInstrumentId())
        .SetOrderType(new_order_single.GetOrderType())
        .SetOrderPrice(new_order_single.GetOrderPrice())
        .SetOrderQuantity(new_order_single.GetOrderQuantity())
        .SetLeavesQuantity(new_order_single.GetOrderQuantity())
        .SetSide(new_order_single.GetSide())
        .SetTimeInForce(new_order_single.GetTimeInForce())
        .SetClientOrderId(new_order_single.GetClientOrderId())
        .SetOrderStatus(OrderStatus::kPendingNew)
        .SetExecutedQuantity(0)
        .SetLastPrice(0)
        .SetLastQuantity(0)
        .SetExecutedValue(0);
    return ord;
  }

  static auto MakeOrder(const Price& price, const Quantity& quantity,
                        const Side& side) -> OrderPtr {
    NewOrderSingle nos;
    nos.SetRoutingId(0);
    nos.SetSessionId(0);
    nos.SetAccountId(0);
    nos.SetInstrumentId(1);
    nos.SetClientOrderId(RandomStringView(kClientOrderIdSize));
    nos.SetOrderType(OrderTypeCode::kLimit);
    nos.SetTimeInForce(TimeInForceCode::kDay);
    nos.SetOrderPrice(price);
    nos.SetOrderQuantity(quantity);
    nos.SetSide(side);
    return MakeOrder(nos);
  }

  static auto MakeCancel(const OrderPtr& order) -> OrderCancelRequest {
    OrderCancelRequest request;
    request.SetOrderId(order->GetOrderId());
    request.SetSide(order->GetSide());
    request.SetOrderPrice(order->GetOrderPrice());
    request.SetOrderQuantity(order->GetOrderQuantity());
    request.SetClientOrderId(order->GetClientOrderId());
    return request;
  }

 public:
  static auto EmptyTest() -> void {
    BidContainer container;
    ASSERT_TRUE(container.IsEmpty());
  }

  static auto AddTest() -> void {
    BidContainer container;

    auto order = MakeOrder(456,  // NOLINT
                           33,   // NOLINT
                           SideCode::kBuy);

    ASSERT_TRUE(order->GetRef() == 1);
    ASSERT_TRUE(container.Add(order));
    ASSERT_FALSE(container.IsEmpty());

    auto& resting_order = container.Front();
    ASSERT_TRUE(resting_order->GetOrderQuantity() == order->GetOrderQuantity());
    ASSERT_TRUE(resting_order->GetOrderPrice() == order->GetOrderPrice());
    ASSERT_TRUE(resting_order->GetClientOrderId() == order->GetClientOrderId());
    ASSERT_TRUE(resting_order->GetSide() == order->GetSide());
  }

  static auto RemoveTest() -> void {
    AskContainer container;

    auto order = MakeOrder(510,  // NOLINT
                           81,   // NOLINT
                           SideCode::kSell);

    ASSERT_TRUE(order->GetRef() == 1);
    ASSERT_TRUE(container.Add(order));
    ASSERT_TRUE(order->GetRef() == 2);
    ASSERT_FALSE(container.IsEmpty());

    auto cancel = MakeCancel(order);

    auto&& [removed, removed_order] = container.Remove(cancel);
    ASSERT_TRUE(removed);
    ASSERT_TRUE(removed_order->GetOrderId() == order->GetOrderId());
    ASSERT_TRUE(order->GetRef() == 2);
    ASSERT_TRUE(removed_order->GetRef() == 2);
  }

  static auto PriceLevelTest() -> void {
    BidContainer bids;
    AskContainer asks;

    ASSERT_TRUE(bids.Add(MakeOrder(20, 10, SideCode::kBuy)));  // NOLINT
    ASSERT_TRUE(bids.Add(MakeOrder(19, 10, SideCode::kBuy)));  // NOLINT
    ASSERT_TRUE(bids.Add(MakeOrder(21, 10, SideCode::kBuy)));  // NOLINT
    ASSERT_TRUE(bids.Add(MakeOrder(20, 10, SideCode::kBuy)));  // NOLINT

    ASSERT_TRUE(asks.Add(MakeOrder(20, 10, SideCode::kSell)));  // NOLINT
    ASSERT_TRUE(asks.Add(MakeOrder(19, 10, SideCode::kSell)));  // NOLINT
    ASSERT_TRUE(asks.Add(MakeOrder(21, 10, SideCode::kSell)));  // NOLINT
    ASSERT_TRUE(asks.Add(MakeOrder(20, 10, SideCode::kSell)));  // NOLINT

    ASSERT_TRUE(bids.Count() == 4);  // NOLINT
    ASSERT_TRUE(asks.Count() == 4);  // NOLINT

    ASSERT_TRUE(bids.Front()->GetOrderPrice() == 21);  // NOLINT
    ASSERT_TRUE(asks.Front()->GetOrderPrice() == 19);  // NOLINT

    // IMPORTANT! -- notice how we're using OrderPtr and not OrderCancelRequest
    bids.Remove(*bids.Front());
    asks.Remove(*asks.Front());

    ASSERT_TRUE(bids.Count() == 3);  // NOLINT
    ASSERT_TRUE(asks.Count() == 3);  // NOLINT

    ASSERT_TRUE(bids.Front()->GetOrderPrice() == 20);  // NOLINT
    ASSERT_TRUE(asks.Front()->GetOrderPrice() == 20);  // NOLINT
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
