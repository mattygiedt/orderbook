#include <random>

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"
#include "orderbook/application_traits.h"
#include "orderbook/data/limit_order.h"
#include "spdlog/spdlog.h"

using namespace orderbook::data;

using LimitOrderVec = std::vector<LimitOrder>;

constexpr auto kPoolSize = 16;
constexpr auto kClientOrderIdSize = 8;
constexpr int kMaxPrc = 75;
constexpr int kMinPrc = 25;
constexpr int kMaxQty = 500;
constexpr int kMinQty = 100;

OrderId order_id{0};

auto MakeClientOrderId(const std::size_t& length) -> std::string {
  static std::size_t clord_id{0};
  const auto& id = std::to_string(++clord_id);
  auto clord_id_str =
      std::string(length - std::min(length, id.length()), '0') + id;
  return clord_id_str;
}

auto NextRandom(const int max, const int min) -> int {
  return rand() % (max - min + 1) + min;
}

auto NextSide() -> SideCode {
  static auto gen = std::bind(std::uniform_int_distribution<>(0, 1),  // NOLINT
                              std::default_random_engine());          // NOLINT
  return (gen() == 0) ? SideCode::kBuy : SideCode::kSell;
}

auto MakeNewOrderSingle() -> NewOrderSingle {
  auto nos = NewOrderSingle();
  nos.SetRoutingId(0)
      .SetSessionId(1)
      .SetAccountId(1)
      .SetInstrumentId(16)  // NOLINT
      .SetClientOrderId(MakeClientOrderId(kClientOrderIdSize))
      .SetOrderType(OrderTypeCode::kLimit)
      .SetTimeInForce(TimeInForceCode::kDay)
      .SetOrderPrice(NextRandom(kMaxPrc, kMinPrc))
      .SetOrderQuantity(NextRandom(kMaxQty, kMinQty))
      .SetSide(SideCode::kBuy);
  return nos;
}

auto ModifyOrder(LimitOrder& order) -> OrderCancelReplaceRequest {
  const auto& orig_clordid = order.GetClientOrderId();
  order.SetClientOrderId(MakeClientOrderId(kClientOrderIdSize))
      .SetOrigClientOrderId(orig_clordid)
      .SetOrderPrice(NextRandom(kMaxPrc, kMinPrc))
      .SetOrderQuantity(NextRandom(kMaxQty, kMinQty));

  auto ocrr = OrderCancelReplaceRequest();
  ocrr.SetOrderId(order.GetOrderId())
      .SetSide(order.GetSide())
      .SetOrderType(order.GetOrderType())
      .SetOrderPrice(order.GetOrderPrice())
      .SetOrderQuantity(order.GetOrderQuantity())
      .SetSessionId(order.GetSessionId())
      .SetAccountId(order.GetAccountId())
      .SetInstrumentId(order.GetInstrumentId())
      .SetClientOrderId(order.GetClientOrderId())
      .SetOrigClientOrderId(order.GetOrigClientOrderId());

  return ocrr;
}

auto CancelOrder(LimitOrder& order) -> OrderCancelRequest {
  const auto& orig_clordid = order.GetClientOrderId();
  order.SetClientOrderId(MakeClientOrderId(kClientOrderIdSize));
  order.SetOrigClientOrderId(orig_clordid);

  auto ocxl = OrderCancelRequest();
  ocxl.SetOrderId(order.GetOrderId())
      .SetSide(order.GetSide())
      .SetOrderQuantity(order.GetOrderQuantity())
      .SetSessionId(order.GetSessionId())
      .SetAccountId(order.GetAccountId())
      .SetInstrumentId(order.GetInstrumentId())
      .SetClientOrderId(order.GetClientOrderId())
      .SetOrigClientOrderId(order.GetOrigClientOrderId());

  return ocxl;
}

template <typename Container>
static void BM_AddModifyDeleteOrder_Debug(benchmark::State& state) {
  spdlog::set_pattern("[%H:%M:%S.%f] [thr %t] %v");

  LimitOrderVec order_vec;
  Container container;

  std::size_t run = 0;

  for (auto _ : state) {
    spdlog::info("begin benchmark run: {}", ++run);

    order_vec.resize(kPoolSize);
    for (std::size_t i = 0; i < kPoolSize; ++i) {
      auto nos = MakeNewOrderSingle();
      auto&& [added, order] = container.Add(nos, ++order_id);
      if (!added) {
        spdlog::error("container.Add(nos, {}): returned false", order_id);
        return;
      }

      order_vec[i] = order;
    }

    spdlog::info("after add: container.Count() {}", container.Count());
    spdlog::info("after add:  order_vec.size() {}", order_vec.size());

    for (auto& resting_order : order_vec) {
      auto ocrr = ModifyOrder(resting_order);
      auto&& [modified, order] = container.Modify(ocrr);
      if (modified) {
        spdlog::info(" container.Modified(order_id: {}), count: {}",
                     ocrr.GetOrderId(), container.Count());
      } else {
        spdlog::error("container.Modify(ocrr): returned false");
        return;
      }
    }

    for (auto& resting_order : order_vec) {
      auto ocxl = CancelOrder(resting_order);
      auto&& [removed, order] = container.Remove(ocxl);

      if (removed) {
        spdlog::info(" container.Removed(order_id: {}), count: {}",
                     ocxl.GetOrderId(), container.Count());
      } else {
        spdlog::error(" container.Remove(ocxl, {}): returned false",
                      ocxl.GetOrderId());
        return;
      }
    }

    spdlog::info("after cxl: container.Count() {}", container.Count());

    container.Clear();
    order_vec.clear();

    spdlog::info("after clr: container.Count() {}", container.Count());
  }

  spdlog::info("total benchmark runs: {}", run);
  spdlog::info("last order_id: {}", order_id);
}

template <typename Container>
static void BM_AddModifyDeleteOrder(benchmark::State& state) {
  LimitOrderVec order_vec;
  Container container;

  for (auto _ : state) {
    order_vec.resize(kPoolSize);
    for (std::size_t i = 0; i < kPoolSize; ++i) {
      auto nos = MakeNewOrderSingle();
      auto&& [added, order] = container.Add(nos, ++order_id);
      if (!added) {
        spdlog::error("container.Add(nos, {}): returned false", order_id);
        return;
      }

      order_vec[i] = order;
    }

    for (auto& resting_order : order_vec) {
      auto ocrr = ModifyOrder(resting_order);
      auto&& [modified, order] = container.Modify(ocrr);
      if (!modified) {
        spdlog::error("container.Modify(ocrr): returned false");
        return;
      }
    }

    for (auto& resting_order : order_vec) {
      auto ocxl = CancelOrder(resting_order);
      auto&& [removed, order] = container.Remove(ocxl);

      if (!removed) {
        spdlog::error(" container.Remove(ocxl, {}): returned false",
                      ocxl.GetOrderId());
        return;
      }
    }

    container.Clear();
    order_vec.clear();
  }
}

using MapListTraits = typename orderbook::MapListOrderBookTraits;
using IntrusivePtrTraits =
    typename orderbook::IntrusivePtrOrderBookTraits<kPoolSize>;
using IntrusiveListTraits =
    typename orderbook::IntrusiveListOrderBookTraits<kPoolSize>;

// BENCHMARK(BM_AddModifyDeleteOrder_Debug<
//           typename MapListTraits::AskContainerType>);

// BENCHMARK(BM_AddModifyDeleteOrder_Debug<
//           typename IntrusivePtrTraits::AskContainerType>);

// BENCHMARK(BM_AddModifyDeleteOrder_Debug<
//           typename IntrusiveListTraits::AskContainerType>);

BENCHMARK(BM_AddModifyDeleteOrder<typename MapListTraits::BidContainerType>);
BENCHMARK(BM_AddModifyDeleteOrder<typename MapListTraits::AskContainerType>);

BENCHMARK(
    BM_AddModifyDeleteOrder<typename IntrusivePtrTraits::BidContainerType>);
BENCHMARK(
    BM_AddModifyDeleteOrder<typename IntrusivePtrTraits::AskContainerType>);

BENCHMARK(
    BM_AddModifyDeleteOrder<typename IntrusiveListTraits::BidContainerType>);
BENCHMARK(
    BM_AddModifyDeleteOrder<typename IntrusiveListTraits::AskContainerType>);

BENCHMARK_MAIN();  // NOLINT
