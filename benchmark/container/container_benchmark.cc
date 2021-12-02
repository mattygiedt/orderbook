#include <random>

#include "benchmark/benchmark.h"
#include "orderbook/application_traits.h"
#include "spdlog/spdlog.h"

using namespace orderbook::data;

constexpr auto kPoolSize = 16;
constexpr auto kClientOrderIdSize = 8;
constexpr int kMaxPrc = 75;
constexpr int kMinPrc = 25;
constexpr int kMaxQty = 500;
constexpr int kMinQty = 100;

using IntrusiveOrder = orderbook::data::IntrusiveLimitOrder<kPoolSize>;
using IntrusiveOrderPool =
    orderbook::data::IntrusivePool<IntrusiveOrder,
                                   IntrusiveOrder::GetPoolSize()>;
using NewOrderSingleVec = std::vector<NewOrderSingle>;
using MapListKey = orderbook::data::Price;
using MapListBidContainer =
    orderbook::container::MapListContainer<MapListKey, IntrusiveOrder,
                                           IntrusiveOrderPool, std::greater<>>;
using MapListAskContainer =
    orderbook::container::MapListContainer<MapListKey, IntrusiveOrder,
                                           IntrusiveOrderPool, std::less<>>;

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
static void BM_AddIntrusiveOrder(benchmark::State& state) {
  NewOrderSingleVec vec;
  Container container;

  vec.resize(kPoolSize);
  for (auto i = 0; i < kPoolSize; ++i) {
    vec[i] = MakeNewOrderSingle();
  }

  for (auto _ : state) {
    for (auto&& new_order : vec) {
      if (!container.Add(new_order, ++order_id).first) {
        spdlog::error("container.Add(order): returned false");
      }
    }
    container.Clear();
  }
}

BENCHMARK(BM_AddIntrusiveOrder<MapListBidContainer>);
BENCHMARK(BM_AddIntrusiveOrder<MapListAskContainer>);

BENCHMARK_MAIN();  // NOLINT
