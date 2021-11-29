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

  static auto MakeNewOrderSingle(const Price& price, const Quantity& quantity,
                                 const Side& side) -> NewOrderSingle {
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
    return nos;
  }

  static auto MakeCancel(const NewOrderSingle& order, const OrderId id)
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

TEST_F(MapListOrderBookFixture, simple_execute_test) {  // NOLINT
  SimpleExecuteTest();
}

TEST_F(MapListOrderBookFixture, partial_execute_test) {  // NOLINT
  PartialExecuteTest();
}

TEST_F(MapListOrderBookFixture, cancel_test) { CancelTest(); }  // NOLINT
