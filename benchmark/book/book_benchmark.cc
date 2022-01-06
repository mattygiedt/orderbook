#include "benchmark/benchmark.h"
#include "utils.h"

// constexpr auto kMaxBookSize = 1048576;
constexpr auto kMaxBookSize = 131072;

using MapListTraits = typename orderbook::MapListOrderBookTraits;
using IntrusivePtrTraits =
    typename orderbook::IntrusivePtrOrderBookTraits<kMaxBookSize>;
using IntrusiveListTraits =
    typename orderbook::IntrusiveListOrderBookTraits<kMaxBookSize>;

struct BookEventCounter {
  std::size_t buy_order_pending_new{0};
  std::size_t sell_order_pending_new{0};
  std::size_t buy_order_qty{0};
  std::size_t sell_order_qty{0};
  std::size_t buy_order_executed_qty{0};
  std::size_t sell_order_executed_qty{0};

  std::size_t order_pending_new{0};
  std::size_t order_new{0};
  std::size_t order_partially_filled{0};
  std::size_t order_filled{0};
  std::size_t order_cancelled{0};
  std::size_t order_rejected{0};
  std::size_t order_modified{0};
  std::size_t order_cancel_rejected{0};
  std::size_t total_events{0};
};

template <typename OrderBookTraits>
static void BM_OrderBook(benchmark::State& state) {
  using BookType = typename OrderBookTraits::BookType;
  using EventType = typename OrderBookTraits::EventType;
  using EventData = typename OrderBookTraits::EventData;
  using EventDispatcher = typename OrderBookTraits::EventDispatcher;
  using EventDispatcherPtr = std::shared_ptr<EventDispatcher>;

  EventDispatcherPtr dispatcher = std::make_shared<EventDispatcher>();
  BookType book = BookType(dispatcher);
  BookEventCounter counter;

  dispatcher->appendListener(
      EventType::kOrderPendingNew, [&](const EventData& data) {
        counter.order_pending_new += 1;
        counter.total_events += 1;
        auto& execution_report = std::get<ExecutionReport>(data);
        execution_report.GetSide() == SideCode::kBuy
            ? counter.buy_order_pending_new += 1
            : counter.sell_order_pending_new += 1;
      });

  dispatcher->appendListener(EventType::kOrderNew, [&](const EventData& data) {
    counter.order_new += 1;
    counter.total_events += 1;
    auto& execution_report = std::get<ExecutionReport>(data);
    execution_report.GetSide() == SideCode::kBuy
        ? counter.buy_order_qty += execution_report.GetOrderQuantity()
        : counter.sell_order_qty += execution_report.GetOrderQuantity();
  });

  dispatcher->appendListener(
      EventType::kOrderPartiallyFilled, [&](const EventData& data) {
        counter.order_partially_filled += 1;
        counter.total_events += 1;
        auto& execution_report = std::get<ExecutionReport>(data);
        execution_report.GetSide() == SideCode::kBuy
            ? counter.buy_order_executed_qty +=
              execution_report.GetLastQuantity()
            : counter.sell_order_executed_qty +=
              execution_report.GetLastQuantity();
      });

  dispatcher->appendListener(
      EventType::kOrderFilled, [&](const EventData& data) {
        counter.order_filled += 1;
        counter.total_events += 1;
        auto& execution_report = std::get<ExecutionReport>(data);
        execution_report.GetSide() == SideCode::kBuy
            ? counter.buy_order_executed_qty +=
              execution_report.GetLastQuantity()
            : counter.sell_order_executed_qty +=
              execution_report.GetLastQuantity();
      });

  dispatcher->appendListener(EventType::kOrderCancelled,
                             [&](const EventData& /*unused*/) {
                               counter.order_cancelled += 1;
                               counter.total_events += 1;
                             });

  dispatcher->appendListener(EventType::kOrderRejected,
                             [&](const EventData& /*unused*/) {
                               counter.order_rejected += 1;
                               counter.total_events += 1;
                             });

  dispatcher->appendListener(EventType::kOrderModified,
                             [&](const EventData& /*unused*/) {
                               counter.order_modified += 1;
                               counter.total_events += 1;
                             });

  dispatcher->appendListener(EventType::kOrderCancelRejected,
                             [&](const EventData& /*unused*/) {
                               counter.order_cancel_rejected += 1;
                               counter.total_events += 1;
                             });

  for (auto _ : state) {
    auto nos = MakeNewOrderSingle(NextSide());
    book.Add(nos);
  }

  // book.CancelAll(kSessionId);

  // state.counters["buy_order_pending_new"] = counter.buy_order_pending_new;
  // state.counters["sell_order_pending_new"] = counter.sell_order_pending_new;
  // state.counters["buy_order_qty"] = counter.buy_order_qty;
  // state.counters["sell_order_qty"] = counter.sell_order_qty;
  // state.counters["buy_order_executed_qty"] = counter.buy_order_executed_qty;
  // state.counters["sell_order_executed_qty"] =
  // counter.sell_order_executed_qty; state.counters["order_pending_new"] =
  // counter.order_pending_new; state.counters["order_new"] = counter.order_new;
  // state.counters["order_partially_filled"] = counter.order_partially_filled;
  // state.counters["order_filled"] = counter.order_filled;
  // state.counters["order_cancelled"] = counter.order_cancelled;
  // state.counters["order_rejected"] = counter.order_rejected;
  // state.counters["order_modified"] = counter.order_modified;
  // state.counters["order_cancel_rejected"] = counter.order_cancel_rejected;

  state.counters["total_events"] = counter.total_events;
  state.counters["total_events_rate"] =
      benchmark::Counter(counter.total_events, benchmark::Counter::kIsRate);
  state.counters["total_events_rate_inv"] =
      benchmark::Counter(counter.total_events, benchmark::Counter::kIsRate |
                                                   benchmark::Counter::kInvert);
}

BENCHMARK(BM_OrderBook<MapListTraits>);
BENCHMARK(BM_OrderBook<IntrusivePtrTraits>);
BENCHMARK(BM_OrderBook<IntrusiveListTraits>);

BENCHMARK_MAIN();  // NOLINT
