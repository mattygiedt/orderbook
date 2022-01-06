#include "benchmark/benchmark.h"
#include "gtest/gtest.h"
#include "utils.h"

using namespace orderbook::data;

using LimitOrderVec = std::vector<LimitOrder>;

OrderId order_id{0};
constexpr auto kPoolSize = 16;

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
      auto nos = MakeNewOrderSingle(SideCode::kBuy);
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
      auto nos = MakeNewOrderSingle(SideCode::kBuy);
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
