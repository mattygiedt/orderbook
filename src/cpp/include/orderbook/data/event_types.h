#pragma once

#include <variant>

#include "orderbook/data/empty.h"
#include "orderbook/data/execution_report.h"
#include "orderbook/data/new_order_single.h"
#include "orderbook/data/order_cancel_reject.h"
#include "orderbook/data/order_cancel_replace_request.h"
#include "orderbook/data/order_cancel_request.h"
#include "orderbook/data/reject.h"

namespace orderbook::data {

enum class EventType : std::uint8_t {
  kUnknown = 0,
  kOrderPendingNew = 1,
  kOrderPendingModify = 2,
  kOrderPendingCancel = 3,
  kOrderRejected = 4,
  kOrderNew = 5,
  kOrderPartiallyFilled = 6,
  kOrderFilled = 7,
  kOrderCancelled = 8,
  kOrderCompleted = 9,
  kOrderCancelRejected = 10,
  kOrderModified = 11,
  kCancelOnDisconnect = 12
};

using EventData =
    std::variant<ExecutionReport, NewOrderSingle, OrderCancelRequest,
                 OrderCancelReplaceRequest, OrderCancelReject, Reject, Empty>;
using EventCallback = void(const EventData&);

}  // namespace orderbook::data
