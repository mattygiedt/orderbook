#include <random>

#include "orderbook/application_traits.h"
#include "orderbook/data/limit_order.h"
#include "spdlog/spdlog.h"

using namespace orderbook::data;

constexpr auto kClientOrderIdSize = 8;
constexpr int kMaxPrc = 75;
constexpr int kMinPrc = 25;
constexpr int kMaxQty = 500;
constexpr int kMinQty = 100;

constexpr AccountId kAccountId = 9;
constexpr SessionId kSessionId = 51;
constexpr InstrumentId kInstrumentId = 16;

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

auto MakeNewOrderSingle(const SideCode& side) -> NewOrderSingle {
  auto nos = NewOrderSingle();
  nos.SetRoutingId(0)
      .SetSessionId(kSessionId)
      .SetAccountId(kAccountId)
      .SetInstrumentId(kInstrumentId)
      .SetClientOrderId(MakeClientOrderId(kClientOrderIdSize))
      .SetOrderType(OrderTypeCode::kLimit)
      .SetTimeInForce(TimeInForceCode::kDay)
      .SetOrderPrice(NextRandom(kMaxPrc, kMinPrc))
      .SetOrderQuantity(NextRandom(kMaxQty, kMinQty))
      .SetSide(side);
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
