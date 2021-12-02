#include "gtest/gtest.h"
#include "orderbook/application_traits.h"

using namespace orderbook::data;

// clang-format off
template <typename Traits>

  requires( orderbook::book::BookConcept<typename Traits::BookType> )

class OrderBookFixture : public ::testing::Test {  // clang-format on
 private:
  using Order = typename Traits::OrderType;
  using OrderBook = typename Traits::BookType;
  using EventType = typename Traits::EventType;
  using EventData = typename Traits::EventData;
  using EventCallback = typename Traits::EventCallback;
  using EventDispatcher = typename Traits::EventDispatcher;
  using EventDispatcherPtr = std::shared_ptr<EventDispatcher>;

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

  static auto MakeCancel(const NewOrderSingle& order, const OrderId& id)
      -> OrderCancelRequest {
    OrderCancelRequest request;
    request.SetOrderId(id);
    request.SetSide(order.GetSide());
    request.SetOrderPrice(order.GetOrderPrice());
    request.SetOrderQuantity(order.GetOrderQuantity());
    request.SetClientOrderId(order.GetClientOrderId());
    return request;
  }

  static auto MakeModify(const ExecutionReport& order, const Price& price,
                         const Quantity& quantity)
      -> OrderCancelReplaceRequest {
    OrderCancelReplaceRequest request;
    request.SetOrderId(order.GetOrderId());
    request.SetSide(order.GetSide());
    request.SetOrderPrice(price);
    request.SetOrderQuantity(quantity);
    request.SetOrigClientOrderId(order.GetClientOrderId());
    request.SetClientOrderId(MakeClientOrderId(kClientOrderIdSize));
    return request;
  }

 public:
  static auto AddTest() -> void {
    EventDispatcherPtr dispatcher = std::make_shared<EventDispatcher>();
    OrderBook book{dispatcher};
    std::size_t pending_new_happened{0};
    std::size_t new_happened{0};
    std::size_t reject_happened{0};

    dispatcher->appendListener(
        EventType::kOrderPendingNew,
        [&](const EventData& /*unused*/) { ++pending_new_happened; });

    dispatcher->appendListener(
        EventType::kOrderNew,
        [&](const EventData& /*unused*/) { ++new_happened; });

    dispatcher->appendListener(
        EventType::kOrderRejected,
        [&](const EventData& /*unused*/) { ++reject_happened; });

    const auto& buy_order =
        MakeNewOrderSingle(21, 10, SideCode::kBuy);  // NOLINT
    book.Add(buy_order);
    book.Add(buy_order);

    const auto& sell_order =
        MakeNewOrderSingle(22, 10, SideCode::kSell);  // NOLINT
    book.Add(sell_order);
    book.Add(sell_order);

    ASSERT_FALSE(book.Empty());
    ASSERT_TRUE(pending_new_happened == 4);  // NOLINT
    ASSERT_TRUE(new_happened == 4);          // NOLINT
    ASSERT_TRUE(reject_happened == 0);       // NOLINT
  }

  static auto ModifyTest(const SideCode& side) -> void {
    EventDispatcherPtr dispatcher = std::make_shared<EventDispatcher>();
    OrderBook book{dispatcher};
    ExecutionReport order_ack;
    NewOrderSingle new_order_request;
    OrderCancelReplaceRequest modify_request;

    std::size_t pending_new_happened{0};
    std::size_t new_happened{0};
    std::size_t modify_happened{0};
    std::size_t cancel_reject_happened{0};

    Price last_price{0};
    Quantity last_quantity{0};

    dispatcher->appendListener(
        EventType::kOrderPendingNew,
        [&](const EventData& /*unused*/) { ++pending_new_happened; });

    dispatcher->appendListener(EventType::kOrderNew,
                               [&](const EventData& data) {
                                 ++new_happened;
                                 order_ack = std::get<ExecutionReport>(data);
                               });

    dispatcher->appendListener(
        EventType::kOrderModified, [&](const EventData& data) {
          order_ack = std::get<ExecutionReport>(data);
          ++modify_happened;
          last_price = order_ack.GetOrderPrice();
          last_quantity = order_ack.GetOrderQuantity();
          auto clord_id = std::string(order_ack.GetClientOrderId());
          auto orig_clord_id = std::string(order_ack.GetClientOrderId());

          spdlog::info(
              "kOrderModified: order_id {}, clord_id {}, orig_clord_id {}, prc "
              "{}, qty {}",
              order_ack.GetOrderId(), clord_id, orig_clord_id, last_price,
              last_quantity);
        });

    dispatcher->appendListener(
        EventType::kOrderCancelRejected,
        [&](const EventData& /*unused*/) { ++cancel_reject_happened; });

    new_order_request = MakeNewOrderSingle(21, 10, side);  // NOLINT
    spdlog::info("NewOrderSingle clord_id {} '{}'",
                 new_order_request.GetClientOrderId().size(),
                 new_order_request.GetClientOrderId());
    book.Add(new_order_request);

    // Change price
    modify_request = MakeModify(order_ack, 22, 10);  // NOLINT
    book.Modify(modify_request);
    ASSERT_TRUE(cancel_reject_happened == 0);  // NOLINT
    ASSERT_TRUE(pending_new_happened == 1);    // NOLINT
    ASSERT_TRUE(new_happened == 1);            // NOLINT
    ASSERT_TRUE(modify_happened == 1);         // NOLINT
    ASSERT_TRUE(last_price == 22);             // NOLINT
    ASSERT_TRUE(last_quantity == 10);          // NOLINT

    // Quantity up
    modify_request = MakeModify(order_ack, 22, 11);  // NOLINT
    book.Modify(modify_request);
    ASSERT_TRUE(cancel_reject_happened == 0);  // NOLINT
    ASSERT_TRUE(pending_new_happened == 1);    // NOLINT
    ASSERT_TRUE(new_happened == 1);            // NOLINT
    ASSERT_TRUE(modify_happened == 2);         // NOLINT
    ASSERT_TRUE(last_price == 22);             // NOLINT
    ASSERT_TRUE(last_quantity == 11);          // NOLINT

    // Quantity down
    modify_request = MakeModify(order_ack, 22, 9);  // NOLINT
    book.Modify(modify_request);
    ASSERT_TRUE(cancel_reject_happened == 0);  // NOLINT
    ASSERT_TRUE(pending_new_happened == 1);    // NOLINT
    ASSERT_TRUE(new_happened == 1);            // NOLINT
    ASSERT_TRUE(modify_happened == 3);         // NOLINT
    ASSERT_TRUE(last_price == 22);             // NOLINT
    ASSERT_TRUE(last_quantity == 9);           // NOLINT

    // Change price and quantity
    modify_request = MakeModify(order_ack, 10, 10);  // NOLINT
    book.Modify(modify_request);
    ASSERT_TRUE(cancel_reject_happened == 0);  // NOLINT
    ASSERT_TRUE(pending_new_happened == 1);    // NOLINT
    ASSERT_TRUE(new_happened == 1);            // NOLINT
    ASSERT_TRUE(modify_happened == 4);         // NOLINT
    ASSERT_TRUE(last_price == 10);             // NOLINT
    ASSERT_TRUE(last_quantity == 10);          // NOLINT

    // Try to modify an 'old' order
    book.Modify(modify_request);
    ASSERT_TRUE(cancel_reject_happened == 1);  // NOLINT
    ASSERT_TRUE(pending_new_happened == 1);    // NOLINT
    ASSERT_TRUE(new_happened == 1);            // NOLINT
    ASSERT_TRUE(modify_happened == 4);         // NOLINT
    ASSERT_TRUE(last_price == 10);             // NOLINT
    ASSERT_TRUE(last_quantity == 10);          // NOLINT
  }

  static auto SimpleExecuteTest() -> void {
    EventDispatcherPtr dispatcher = std::make_shared<EventDispatcher>();
    OrderBook book{dispatcher};

    std::size_t pending_new_happened{0};
    std::size_t new_happened{0};
    std::size_t reject_happened{0};
    std::size_t partial_exec_happened{0};
    std::size_t filled_exec_happened{0};

    dispatcher->appendListener(
        EventType::kOrderPendingNew,
        [&](const EventData& /*unused*/) { ++pending_new_happened; });

    dispatcher->appendListener(
        EventType::kOrderNew,
        [&](const EventData& /*unused*/) { ++new_happened; });

    dispatcher->appendListener(
        EventType::kOrderRejected,
        [&](const EventData& /*unused*/) { ++reject_happened; });

    dispatcher->appendListener(
        EventType::kOrderPartiallyFilled,
        [&](const EventData& /*unused*/) { ++partial_exec_happened; });

    dispatcher->appendListener(
        EventType::kOrderFilled, [&](const EventData& data) {
          ++filled_exec_happened;
          auto& exec = std::get<ExecutionReport>(data);
          ASSERT_TRUE(exec.GetLeavesQuantity() == 0);  // NOLINT
          ASSERT_TRUE(exec.GetLastQuantity() == 10);   // NOLINT
          ASSERT_TRUE(exec.GetOrderQuantity() == 10);  // NOLINT
          ASSERT_TRUE(exec.GetLastPrice() == 21);      // NOLINT
          ASSERT_TRUE(exec.GetOrderStatus() == OrderStatus::kFilled);
        });

    const auto& buy_order =
        MakeNewOrderSingle(21, 10, SideCode::kBuy);  // NOLINT
    book.Add(buy_order);

    const auto& sell_order =
        MakeNewOrderSingle(21, 10, SideCode::kSell);  // NOLINT
    book.Add(sell_order);

    ASSERT_TRUE(book.Empty());
    ASSERT_TRUE(pending_new_happened == 2);   // NOLINT
    ASSERT_TRUE(new_happened == 2);           // NOLINT
    ASSERT_TRUE(reject_happened == 0);        // NOLINT
    ASSERT_TRUE(partial_exec_happened == 0);  // NOLINT
    ASSERT_TRUE(filled_exec_happened == 2);   // NOLINT
  }

  static auto PartialExecuteTest() -> void {
    EventDispatcherPtr dispatcher = std::make_shared<EventDispatcher>();
    OrderBook book{dispatcher};

    std::size_t pending_new_happened{0};
    std::size_t new_happened{0};
    std::size_t reject_happened{0};
    std::size_t partial_exec_happened{0};
    std::size_t filled_exec_happened{0};

    dispatcher->appendListener(
        EventType::kOrderPendingNew,
        [&](const EventData& /*unused*/) { ++pending_new_happened; });

    dispatcher->appendListener(
        EventType::kOrderNew,
        [&](const EventData& /*unused*/) { ++new_happened; });

    dispatcher->appendListener(
        EventType::kOrderRejected,
        [&](const EventData& /*unused*/) { ++reject_happened; });

    dispatcher->appendListener(
        EventType::kOrderPartiallyFilled, [&](const EventData& data) {
          ++partial_exec_happened;
          auto& exec = std::get<ExecutionReport>(data);
          ASSERT_TRUE(exec.GetLeavesQuantity() == 5);  // NOLINT
          ASSERT_TRUE(exec.GetLastQuantity() == 5);    // NOLINT
          ASSERT_TRUE(exec.GetOrderQuantity() == 10);  // NOLINT
          ASSERT_TRUE(exec.GetLastPrice() == 21);      // NOLINT
        });

    dispatcher->appendListener(
        EventType::kOrderFilled,
        [&](const EventData& /*unused*/) { ++filled_exec_happened; });

    const auto& buy_order =
        MakeNewOrderSingle(21, 10, SideCode::kBuy);  // NOLINT
    book.Add(buy_order);

    const auto& sell_order =
        MakeNewOrderSingle(21, 5, SideCode::kSell);  // NOLINT
    book.Add(sell_order);
    book.Add(sell_order);

    ASSERT_TRUE(book.Empty());
    ASSERT_TRUE(pending_new_happened == 3);   // NOLINT
    ASSERT_TRUE(new_happened == 3);           // NOLINT
    ASSERT_TRUE(reject_happened == 0);        // NOLINT
    ASSERT_TRUE(partial_exec_happened == 1);  // NOLINT
    ASSERT_TRUE(filled_exec_happened == 3);   // NOLINT
  }

  static auto CancelTest() -> void {
    EventDispatcherPtr dispatcher = std::make_shared<EventDispatcher>();
    OrderBook book{dispatcher};

    std::size_t cancel_happened{0};
    std::size_t cancel_reject_happened{0};

    OrderId buy_order_id{0};
    OrderId sell_order_id{0};

    dispatcher->appendListener(EventType::kOrderNew,
                               [&](const EventData& data) {
                                 auto& exec = std::get<ExecutionReport>(data);
                                 if (exec.GetSide() == SideCode::kBuy) {
                                   buy_order_id = exec.GetOrderId();
                                 } else {
                                   sell_order_id = exec.GetOrderId();
                                 }
                               });

    dispatcher->appendListener(
        EventType::kOrderCancelled,
        [&](const EventData& /*unused*/) { ++cancel_happened; });

    dispatcher->appendListener(
        EventType::kOrderCancelRejected,
        [&](const EventData& /*unused*/) { ++cancel_reject_happened; });

    const auto& buy_order =
        MakeNewOrderSingle(21, 10, SideCode::kBuy);  // NOLINT
    book.Add(buy_order);
    ASSERT_FALSE(book.Empty());
    ASSERT_FALSE(buy_order_id == 0);

    const auto& sell_order =
        MakeNewOrderSingle(33, 5, SideCode::kSell);  // NOLINT
    book.Add(sell_order);
    ASSERT_FALSE(sell_order_id == 0);

    const auto& cancel_buy_order =
        MakeCancel(buy_order, buy_order_id);  // NOLINT
    book.Cancel(cancel_buy_order);

    const auto& cancel_sell_order =
        MakeCancel(sell_order, sell_order_id);  // NOLINT
    book.Cancel(cancel_sell_order);

    ASSERT_TRUE(book.Empty());
    ASSERT_TRUE(cancel_happened == 2);         // NOLINT
    ASSERT_TRUE(cancel_reject_happened == 0);  // NOLINT

    book.Cancel(cancel_sell_order);
    ASSERT_TRUE(cancel_reject_happened == 1);  // NOLINT
  }
};

using MapListOrderBookFixture =
    OrderBookFixture<orderbook::MapListOrderBookTraits>;

TEST_F(MapListOrderBookFixture, add_test) { AddTest(); }  // NOLINT

TEST_F(MapListOrderBookFixture, modify_buy_test) {  // NOLINT
  ModifyTest(SideCode::kBuy);
}
TEST_F(MapListOrderBookFixture, modify_sell_test) {  // NOLINT
  ModifyTest(SideCode::kSell);
}

TEST_F(MapListOrderBookFixture, simple_execute_test) {  // NOLINT
  SimpleExecuteTest();
}

TEST_F(MapListOrderBookFixture, partial_execute_test) {  // NOLINT
  PartialExecuteTest();
}

TEST_F(MapListOrderBookFixture, cancel_test) { CancelTest(); }  // NOLINT
