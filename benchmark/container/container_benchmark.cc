#include <random>

#include "benchmark/benchmark.h"
#include "orderbook/application_traits.h"
#include "spdlog/spdlog.h"

using namespace orderbook::data;

using NewOrderSingleVec = std::vector<NewOrderSingle>;

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
  static NewOrderSingle nos;
  nos.SetRoutingId(0)
      .SetSessionId(0)
      .SetAccountId(0)
      .SetInstrumentId(1)
      .SetClientOrderId(MakeClientOrderId(kClientOrderIdSize))
      .SetOrderType(OrderTypeCode::kLimit)
      .SetTimeInForce(TimeInForceCode::kDay)
      .SetOrderPrice(NextRandom(kMaxPrc, kMinPrc))
      .SetOrderQuantity(NextRandom(kMaxQty, kMinQty))
      .SetSide(NextSide());
  return nos;
}

template <typename Container>
static void BM_AddOrder(benchmark::State& state) {
  NewOrderSingleVec vec;
  Container container;

  vec.resize(Container::GetPoolSize());
  for (std::size_t i = 0; i < Container::GetPoolSize(); ++i) {
    vec[i] = MakeNewOrderSingle();
  }

  for (auto _ : state) {
    for (auto&& new_order : vec) {
      auto&& [added, order] = container.Add(new_order, ++order_id);
      if (!added) {
        spdlog::error("container.Add(order): returned false");
      }
    }
    container.Clear();
  }
}

using MapListTraits = typename orderbook::MapListOrderBookTraits<kPoolSize>;
using IntrusiveListTraits =
    typename orderbook::IntrusiveListOrderBookTraits<kPoolSize>;

BENCHMARK(BM_AddOrder<typename MapListTraits::BidContainerType>);
BENCHMARK(BM_AddOrder<typename MapListTraits::AskContainerType>);

BENCHMARK(BM_AddOrder<typename IntrusiveListTraits::BidContainerType>);
BENCHMARK(BM_AddOrder<typename IntrusiveListTraits::AskContainerType>);

BENCHMARK_MAIN();  // NOLINT
