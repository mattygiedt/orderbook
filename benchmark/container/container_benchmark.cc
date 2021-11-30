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
using IntrusiveOrderPtr = boost::intrusive_ptr<IntrusiveOrder>;
using IntrusiveOrderVec = std::vector<IntrusiveOrderPtr>;
using MapListKey = orderbook::data::Price;
using MapListBidContainer =
    orderbook::container::MapListContainer<MapListKey, IntrusiveOrder,
                                           std::greater<>>;
using MapListAskContainer =
    orderbook::container::MapListContainer<MapListKey, IntrusiveOrder,
                                           std::less<>>;

OrderId order_id{0};
IntrusiveOrderPool& intrusive_pool = IntrusiveOrderPool::Instance();

auto RandomStringView(const std::size_t length) -> std::string_view {
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
      .SetClientOrderId(RandomStringView(kClientOrderIdSize))
      .SetOrderType(OrderTypeCode::kLimit)
      .SetTimeInForce(TimeInForceCode::kDay)
      .SetOrderPrice(NextRandom(kMaxPrc, kMinPrc))
      .SetOrderQuantity(NextRandom(kMaxQty, kMinQty))
      .SetSide(NextSide());
  return nos;
}

auto MakeIntrusiveOrder(const NewOrderSingle& new_order_single)
    -> IntrusiveOrderPtr {
  auto ord = intrusive_pool.MakeIntrusive();
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

template <typename Container>
static void BM_AddRemoveIntrusiveOrder(benchmark::State& state) {
  IntrusiveOrderVec vec;
  Container container;

  vec.resize(kPoolSize);
  for (auto i = 0; i < kPoolSize; ++i) {
    vec[i] = MakeIntrusiveOrder(MakeNewOrderSingle());
  }

  for (auto _ : state) {
    for (auto&& order : vec) {
      if (!container.Add(order)) {
        spdlog::error("container.Add(order): returned false");
      }
    }
    for (auto&& order : vec) {
      if (!container.Remove(*order).first) {
        spdlog::error("container.Remove(order): returned false");
      }
    }
    container.Clear();
  }
}

BENCHMARK(BM_AddRemoveIntrusiveOrder<MapListBidContainer>);
BENCHMARK(BM_AddRemoveIntrusiveOrder<MapListAskContainer>);

BENCHMARK_MAIN();  // NOLINT
