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
  using ReturnPair = std::pair<bool, Order>;
  using OrderPtr = boost::intrusive_ptr<Order>;
  using NewOrderSingle = orderbook::data::NewOrderSingle;

  inline static OrderId order_id{0};
  static constexpr auto kClientOrderIdSize = 8;

  static auto MakeClientOrderId(const std::size_t& length) -> std::string {
    static std::size_t clord_id{0};
    const auto& id = std::to_string(++clord_id);
    auto clord_id_str =
        std::string(length - std::min(length, id.length()), '0') + id;
    return clord_id_str;
  }

  static auto MakeNewOrderSingle(const Price& price, const Quantity& quantity,
                                 const Side& side) -> NewOrderSingle {
    NewOrderSingle nos;
    nos.SetRoutingId(0);
    nos.SetSessionId(0);
    nos.SetAccountId(0);
    nos.SetInstrumentId(1);
    nos.SetClientOrderId(MakeClientOrderId(kClientOrderIdSize));
    nos.SetOrderType(OrderTypeCode::kLimit);
    nos.SetTimeInForce(TimeInForceCode::kDay);
    nos.SetOrderPrice(price);
    nos.SetOrderQuantity(quantity);
    nos.SetSide(side);
    return nos;
  }

  static auto MakeCancel(const Order& order) -> OrderCancelRequest {
    OrderCancelRequest request;
    request.SetOrderId(order.GetOrderId());
    request.SetRoutingId(order.GetRoutingId());
    request.SetSessionId(order.GetSessionId());
    request.SetAccountId(order.GetAccountId());
    request.SetInstrumentId(order.GetInstrumentId());
    request.SetSide(order.GetSide());
    request.SetOrderPrice(order.GetOrderPrice());
    request.SetOrderQuantity(order.GetOrderQuantity());
    request.SetOrigClientOrderId(order.GetClientOrderId());
    request.SetClientOrderId(MakeClientOrderId(kClientOrderIdSize));
    return request;
  }

  static auto MakeModify(const Order& order, const Price& price,
                         const Quantity& quantity)
      -> OrderCancelReplaceRequest {
    OrderCancelReplaceRequest request;
    request.SetOrderId(order.GetOrderId());
    request.SetRoutingId(order.GetRoutingId());
    request.SetSessionId(order.GetSessionId());
    request.SetAccountId(order.GetAccountId());
    request.SetInstrumentId(order.GetInstrumentId());
    request.SetSide(order.GetSide());
    request.SetOrderPrice(price);
    request.SetOrderQuantity(quantity);
    request.SetOrigClientOrderId(order.GetClientOrderId());
    request.SetClientOrderId(MakeClientOrderId(kClientOrderIdSize));
    return request;
  }

 public:
  static auto EmptyTest() -> void {
    BidContainer container;
    ASSERT_TRUE(container.IsEmpty());
  }

  static auto AddTest() -> void {
    BidContainer container;
    ReturnPair return_pair;

    auto new_order = MakeNewOrderSingle(456,  // NOLINT
                                        33,   // NOLINT
                                        SideCode::kBuy);

    return_pair = container.Add(new_order, ++order_id);

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(return_pair.second.GetOrderStatus() == OrderStatus::kNew);
    ASSERT_FALSE(container.IsEmpty());

    auto& resting_order = container.Front();
    ASSERT_TRUE(resting_order.GetOrderQuantity() ==
                return_pair.second.GetOrderQuantity());
    ASSERT_TRUE(resting_order.GetOrderPrice() ==
                return_pair.second.GetOrderPrice());
    ASSERT_TRUE(resting_order.GetClientOrderId() ==
                return_pair.second.GetClientOrderId());
    ASSERT_TRUE(resting_order.GetSide() == return_pair.second.GetSide());

    return_pair = container.Add(new_order, ++order_id);
    ASSERT_FALSE(return_pair.first);
  }

  static auto ModifyPriceTest() -> void {
    Order order;
    BidContainer container;
    ReturnPair return_pair;
    OrderCancelReplaceRequest modify_request;

    constexpr Price price = 456;
    constexpr Price new_price = 256;
    constexpr Quantity quantity = 33;

    // Add the order
    auto new_order = MakeNewOrderSingle(price, quantity, SideCode::kBuy);
    return_pair = container.Add(new_order, ++order_id);
    order = return_pair.second;

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(order.GetOrderStatus() == OrderStatus::kNew);
    ASSERT_TRUE(order.GetOrderId() == order_id);
    ASSERT_FALSE(container.IsEmpty());

    // Modify the order price
    modify_request = MakeModify(order, new_price, quantity);
    ASSERT_TRUE(new_price == modify_request.GetOrderPrice());
    ASSERT_TRUE(order.GetOrderId() == modify_request.GetOrderId());
    ASSERT_FALSE(order.GetClientOrderId() == modify_request.GetClientOrderId());
    ASSERT_TRUE(order.GetClientOrderId() ==
                modify_request.GetOrigClientOrderId());

    return_pair = container.Modify(modify_request);
    order = return_pair.second;

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(order.GetOrderId() == modify_request.GetOrderId());
    ASSERT_TRUE(order.GetClientOrderId() == modify_request.GetClientOrderId());
    ASSERT_TRUE(order.GetOrigClientOrderId() ==
                modify_request.GetOrigClientOrderId());
    ASSERT_TRUE(order.GetOrderPrice() == modify_request.GetOrderPrice());
    ASSERT_TRUE(order.GetOrderQuantity() == modify_request.GetOrderQuantity());

    // Modify the order price (again)
    modify_request = MakeModify(order, new_price + price, quantity);
    ASSERT_TRUE((new_price + price) == modify_request.GetOrderPrice());
    ASSERT_TRUE(order.GetOrderId() == modify_request.GetOrderId());
    ASSERT_FALSE(order.GetClientOrderId() == modify_request.GetClientOrderId());
    ASSERT_TRUE(order.GetClientOrderId() ==
                modify_request.GetOrigClientOrderId());

    return_pair = container.Modify(modify_request);
    order = return_pair.second;

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(order.GetOrderId() == modify_request.GetOrderId());
    ASSERT_TRUE(order.GetClientOrderId() == modify_request.GetClientOrderId());
    ASSERT_TRUE(order.GetOrigClientOrderId() ==
                modify_request.GetOrigClientOrderId());
    ASSERT_TRUE(order.GetOrderPrice() == modify_request.GetOrderPrice());
    ASSERT_TRUE(order.GetOrderQuantity() == modify_request.GetOrderQuantity());
  }

  static auto ModifyQuantityTest(const Quantity& delta) -> void {
    Order order;
    BidContainer container;
    ReturnPair return_pair;
    OrderCancelReplaceRequest modify_request;

    constexpr Price price = 456;
    constexpr Quantity quantity = 3399;
    const Quantity new_quantity = quantity + delta;

    // Add the order
    auto new_order = MakeNewOrderSingle(price, quantity, SideCode::kBuy);
    return_pair = container.Add(new_order, ++order_id);
    order = return_pair.second;

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(order.GetOrderStatus() == OrderStatus::kNew);
    ASSERT_TRUE(order.GetOrderId() == order_id);
    ASSERT_FALSE(container.IsEmpty());

    // Modify the order quantity
    modify_request = MakeModify(order, price, new_quantity);
    ASSERT_TRUE(new_quantity == modify_request.GetOrderQuantity());
    ASSERT_TRUE(order.GetOrderId() == modify_request.GetOrderId());
    ASSERT_FALSE(order.GetClientOrderId() == modify_request.GetClientOrderId());
    ASSERT_TRUE(order.GetClientOrderId() ==
                modify_request.GetOrigClientOrderId());

    return_pair = container.Modify(modify_request);
    order = return_pair.second;

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(order.GetOrderId() == modify_request.GetOrderId());
    ASSERT_TRUE(order.GetClientOrderId() == modify_request.GetClientOrderId());
    ASSERT_TRUE(order.GetOrigClientOrderId() ==
                modify_request.GetOrigClientOrderId());
    ASSERT_TRUE(order.GetOrderPrice() == modify_request.GetOrderPrice());
    ASSERT_TRUE(order.GetOrderQuantity() == modify_request.GetOrderQuantity());

    // Modify the order quantity (again)
    modify_request = MakeModify(order, price, new_quantity + delta);
    ASSERT_TRUE((new_quantity + delta) == modify_request.GetOrderQuantity());
    ASSERT_TRUE(order.GetOrderId() == modify_request.GetOrderId());
    ASSERT_FALSE(order.GetClientOrderId() == modify_request.GetClientOrderId());
    ASSERT_TRUE(order.GetClientOrderId() ==
                modify_request.GetOrigClientOrderId());

    return_pair = container.Modify(modify_request);
    order = return_pair.second;

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(order.GetOrderId() == modify_request.GetOrderId());
    ASSERT_TRUE(order.GetClientOrderId() == modify_request.GetClientOrderId());
    ASSERT_TRUE(order.GetOrigClientOrderId() ==
                modify_request.GetOrigClientOrderId());
    ASSERT_TRUE(order.GetOrderPrice() == modify_request.GetOrderPrice());
    ASSERT_TRUE(order.GetOrderQuantity() == modify_request.GetOrderQuantity());
  }

  static auto ModifyPriceAndQuantityTest() -> void {
    Order order;
    BidContainer container;
    ReturnPair return_pair;
    OrderCancelReplaceRequest modify_request;

    constexpr Price price = 456;
    constexpr Price new_price = 46;
    constexpr Quantity quantity = 33;
    constexpr Quantity new_quantity = 913;

    // Add the order
    auto new_order = MakeNewOrderSingle(price, quantity, SideCode::kBuy);
    return_pair = container.Add(new_order, ++order_id);
    order = return_pair.second;

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(order.GetOrderStatus() == OrderStatus::kNew);
    ASSERT_TRUE(order.GetOrderId() == order_id);
    ASSERT_FALSE(container.IsEmpty());

    // Modify both price and quantity
    modify_request = MakeModify(order, new_price, new_quantity);
    return_pair = container.Modify(modify_request);

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(return_pair.second.GetOrderPrice() ==
                modify_request.GetOrderPrice());
    ASSERT_TRUE(return_pair.second.GetOrderQuantity() ==
                modify_request.GetOrderQuantity());

    // Modify both price and quantity (again)
    modify_request = MakeModify(return_pair.second, new_price + price,
                                new_quantity + quantity);
    return_pair = container.Modify(modify_request);

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(return_pair.second.GetOrderPrice() ==
                modify_request.GetOrderPrice());
    ASSERT_TRUE(return_pair.second.GetOrderQuantity() ==
                modify_request.GetOrderQuantity());
  }

  static auto RemoveTest() -> void {
    AskContainer container;
    ReturnPair return_pair;
    OrderCancelRequest cancel_request;

    constexpr Price price = 456;
    constexpr Quantity quantity = 3399;

    // Add the order
    auto new_order = MakeNewOrderSingle(price, quantity, SideCode::kSell);
    return_pair = container.Add(new_order, ++order_id);

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(return_pair.second.GetOrderStatus() == OrderStatus::kNew);
    ASSERT_FALSE(container.IsEmpty());

    // Cancel the order
    cancel_request = MakeCancel(return_pair.second);
    return_pair = container.Remove(cancel_request);

    ASSERT_TRUE(return_pair.first);
    ASSERT_TRUE(return_pair.second.GetOrderId() == cancel_request.GetOrderId());

    // Cancel the order (again)
    return_pair = container.Remove(cancel_request);
    ASSERT_FALSE(return_pair.first);
  }

  static auto PriceLevelTest() -> void {
    BidContainer bids;
    AskContainer asks;

    bids.Add(MakeNewOrderSingle(20, 10, SideCode::kBuy),  // NOLINT
             ++order_id);
    bids.Add(MakeNewOrderSingle(19, 10, SideCode::kBuy),  // NOLINT
             ++order_id);
    bids.Add(MakeNewOrderSingle(21, 10, SideCode::kBuy),  // NOLINT
             ++order_id);
    bids.Add(MakeNewOrderSingle(20, 10, SideCode::kBuy),  // NOLINT
             ++order_id);

    asks.Add(MakeNewOrderSingle(20, 10, SideCode::kSell),  // NOLINT
             ++order_id);
    asks.Add(MakeNewOrderSingle(19, 10, SideCode::kSell),  // NOLINT
             ++order_id);
    asks.Add(MakeNewOrderSingle(21, 10, SideCode::kSell),  // NOLINT
             ++order_id);
    asks.Add(MakeNewOrderSingle(20, 10, SideCode::kSell),  // NOLINT
             ++order_id);

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

    bids.Remove(bids.Front());
    asks.Remove(asks.Front());

    ASSERT_TRUE(bids.Count() == 2);  // NOLINT
    ASSERT_TRUE(asks.Count() == 2);  // NOLINT

    bids.Remove(bids.Front());
    asks.Remove(asks.Front());

    ASSERT_TRUE(bids.Count() == 1);  // NOLINT
    ASSERT_TRUE(asks.Count() == 1);  // NOLINT

    bids.Remove(bids.Front());
    asks.Remove(asks.Front());

    ASSERT_TRUE(bids.Count() == 0);  // NOLINT
    ASSERT_TRUE(asks.Count() == 0);  // NOLINT

    ASSERT_TRUE(bids.IsEmpty());
    ASSERT_TRUE(asks.IsEmpty());
  }
};

// orderbook::container::MapListContainer tests
using MapListContainerFixture =
    ContainerFixture<orderbook::MapListOrderBookTraits>;

TEST_F(MapListContainerFixture, empty_test) { EmptyTest(); }  // NOLINT

TEST_F(MapListContainerFixture, add_test) { AddTest(); }  // NOLINT

TEST_F(MapListContainerFixture, modify_price_test) {  // NOLINT
  ModifyPriceTest();
}

TEST_F(MapListContainerFixture, modify_quantity_up_test) {  // NOLINT
  ModifyQuantityTest(100);                                  // NOLINT
}

TEST_F(MapListContainerFixture, modify_quantity_down_test) {  // NOLINT
  ModifyQuantityTest(-100);                                   // NOLINT
}

TEST_F(MapListContainerFixture,  // NOLINT
       modify_price_and_quantity_test) {
  ModifyPriceAndQuantityTest();
}

TEST_F(MapListContainerFixture, remove_test) { RemoveTest(); }  // NOLINT

TEST_F(MapListContainerFixture, price_level_test) {  // NOLINT
  PriceLevelTest();
}

// orderbook::container::IntrusivePtrContainer tests
using IntrusivePtrContainerFixture =
    ContainerFixture<orderbook::IntrusivePtrOrderBookTraits<>>;

TEST_F(IntrusivePtrContainerFixture, empty_test) { EmptyTest(); }  // NOLINT

TEST_F(IntrusivePtrContainerFixture, add_test) { AddTest(); }  // NOLINT

TEST_F(IntrusivePtrContainerFixture, modify_price_test) {  // NOLINT
  ModifyPriceTest();
}
TEST_F(IntrusivePtrContainerFixture, modify_quantity_up_test) {  // NOLINT
  ModifyQuantityTest(100);                                       // NOLINT
}
TEST_F(IntrusivePtrContainerFixture, modify_quantity_down_test) {  // NOLINT
  ModifyQuantityTest(-100);                                        // NOLINT
}
TEST_F(IntrusivePtrContainerFixture,  // NOLINT
       modify_price_and_quantity_test) {
  ModifyPriceAndQuantityTest();
}

TEST_F(IntrusivePtrContainerFixture, remove_test) { RemoveTest(); }  // NOLINT

TEST_F(IntrusivePtrContainerFixture, price_level_test) {  // NOLINT
  PriceLevelTest();
}

// orderbook::container::IntrusiveListContainer tests
using IntrusiveListContainerFixture =
    ContainerFixture<orderbook::IntrusiveListOrderBookTraits<>>;

TEST_F(IntrusiveListContainerFixture, empty_test) { EmptyTest(); }  // NOLINT

TEST_F(IntrusiveListContainerFixture, add_test) { AddTest(); }  // NOLINT

TEST_F(IntrusiveListContainerFixture, modify_price_test) {  // NOLINT
  ModifyPriceTest();
}
TEST_F(IntrusiveListContainerFixture, modify_quantity_up_test) {  // NOLINT
  ModifyQuantityTest(100);                                        // NOLINT
}
TEST_F(IntrusiveListContainerFixture, modify_quantity_down_test) {  // NOLINT
  ModifyQuantityTest(-100);                                         // NOLINT
}
TEST_F(IntrusiveListContainerFixture,  // NOLINT
       modify_price_and_quantity_test) {
  ModifyPriceAndQuantityTest();
}

TEST_F(IntrusiveListContainerFixture, remove_test) { RemoveTest(); }  // NOLINT

TEST_F(IntrusiveListContainerFixture, price_level_test) {  // NOLINT
  PriceLevelTest();
}
