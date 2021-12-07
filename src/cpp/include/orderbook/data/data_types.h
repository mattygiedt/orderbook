#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "orderbook/serialize/orderbook_generated.h"
#include "orderbook/util/time_util.h"

namespace orderbook::data {

enum class SideCode : std::uint8_t {
  kUnknown = 0,
  kBuy = 1,
  kSell = 2,
  kSellShort = 3,
  kBuyCover = 4
};

enum class OrderStatusCode : std::uint8_t {
  kUnknown = 0,
  kPendingNew = 1,
  kPendingModify = 2,
  kPendingCancel = 3,
  kRejected = 4,
  kNew = 5,
  kPartiallyFilled = 6,
  kFilled = 7,
  kCancelled = 8,
  kCompleted = 9,
  kCancelRejected = 10
};

enum class TimeInForceCode : std::uint8_t {
  kUnknown = 0,
  kDay = 1,
  kGtc = 2,
  kIoc = 3,  // IOC is immediate-or-cancel
  kFok = 4   // FOK is all-or-none + immediate-or-cancel
};

enum class OrderTypeCode : std::uint8_t {
  kUnknown = 0,
  kMarket = 1,
  kLimit = 2,
  kStop = 3,
  kStopLimit = 4
};

enum class ExecutionTypeCode : std::uint8_t {
  kUnknown = 0,
  kNew = 1,
  kCanceled = 2,
  kModified = 3
};

enum class InstrumentTypeCode : std::uint8_t {
  kUnknown = 0,
  kEquity = 1,
  kFuture = 2,
  kCall = 3,
  kPut = 4
};

enum class CxlRejResponseToCode : std::uint8_t {
  kUnknown = 0,
  kOrderCancelRequest = 1,
  kOrderCancelReplaceRequest = 2
};

using Side = SideCode;
using OrderStatus = OrderStatusCode;
using TimeInForce = TimeInForceCode;
using OrderType = OrderTypeCode;
using ExecutionType = ExecutionTypeCode;
using InstrumentType = InstrumentTypeCode;
using CxlRejResponseTo = CxlRejResponseToCode;

using Price = std::int64_t;
using ExecutedValue = std::int64_t;
using Quantity = std::int32_t;
using ExecutionId = std::uint32_t;
using AccountId = std::uint32_t;
using OrderId = std::uint32_t;
using QuoteId = std::uint32_t;
using RoutingId = std::uint32_t;
using ClientOrderId = std::string;
using OrigClientOrderId = std::string;
using SessionId = std::uint32_t;
using InstrumentId = std::uint64_t;
using TransactionId = std::uint64_t;
using Timestamp = orderbook::util::TimeUtil::Timestamp;

namespace internal {
inline static constexpr int kDoubleToPriceMult = 1000000;
inline static constexpr double kPriceToDoubleMult = 1.0 / kDoubleToPriceMult;
}  // namespace internal

inline auto ToPrice(const double& prc) -> Price {
  return std::floor(prc * internal::kDoubleToPriceMult);
}

inline auto ToDouble(const Price& prc) -> double {
  return prc * internal::kPriceToDoubleMult;
}

class BaseData {
 private:
  using TimeUtil = orderbook::util::TimeUtil;

 public:
  auto& GetTransactionId() const { return transaction_id_; }
  auto& GetCreateTime() const { return create_tm_; }
  auto& GetLastModifyTime() const { return last_modify_tm_; }
  auto Mark() -> BaseData& {
    SetLastModifyTime(TimeUtil::EpochNanos());
    return *this;
  }

  auto GetSide() -> Side { return side_; }
  auto& GetSide() const { return side_; }
  auto GetSerializedSide() const {
    return static_cast<orderbook::serialize::SideCode>(side_);
  }
  auto SetSide(const Side side) -> BaseData& {
    side_ = side;
    return *this;
  }
  auto IsBuyOrder() const -> bool {
    return side_ == SideCode::kBuy || side_ == SideCode::kBuyCover;
  }
  auto IsSellOrder() const -> bool {
    return side_ == SideCode::kSell || side_ == SideCode::kSellShort;
  }

  auto GetOrderStatus() -> OrderStatus { return order_status_; }
  auto& GetOrderStatus() const { return order_status_; }
  auto GetSerializedOrderStatus() const {
    return static_cast<orderbook::serialize::OrderStatusCode>(order_status_);
  }
  auto SetOrderStatus(const OrderStatus order_status) -> BaseData& {
    order_status_ = order_status;
    return *this;
  }
  auto UpdateOrderStatus() -> BaseData& {
    if (executed_quantity_ == order_quantity_) {
      leaves_quantity_ = 0;
      order_status_ = OrderStatus::kFilled;
    } else if (executed_quantity_ > 0) {
      leaves_quantity_ = order_quantity_ - executed_quantity_;
      order_status_ = OrderStatus::kPartiallyFilled;
    } else if (executed_quantity_ == 0) {
      leaves_quantity_ = order_quantity_;
      order_status_ = OrderStatus::kNew;
    }
    return *this;
  }

  auto GetTimeInForce() -> TimeInForce { return time_in_force_; }
  auto& GetTimeInForce() const { return time_in_force_; }
  auto GetSerializedTimeInForce() const {
    return static_cast<orderbook::serialize::TimeInForceCode>(time_in_force_);
  }
  auto SetTimeInForce(const TimeInForce time_in_force) -> BaseData& {
    time_in_force_ = time_in_force;
    return *this;
  }

  auto GetOrderType() -> OrderType { return order_type_; }
  auto& GetOrderType() const { return order_type_; }
  auto GetSerializedOrderType() const {
    return static_cast<orderbook::serialize::OrderTypeCode>(order_type_);
  }
  auto SetOrderType(const OrderType order_type) -> BaseData& {
    order_type_ = order_type;
    return *this;
  }

  auto GetExecutionType() -> ExecutionType { return execution_type_; }
  auto& GetExecutionType() const { return execution_type_; }
  auto GetSerializedExecutionType() const {
    return static_cast<orderbook::serialize::ExecutionTypeCode>(
        execution_type_);
  }
  auto SetExecutionType(const ExecutionType execution_type) -> BaseData& {
    execution_type_ = execution_type;
    return *this;
  }

  auto GetInstrumentType() -> InstrumentType { return instrument_type_; }
  auto& GetInstrumentType() const { return instrument_type_; }
  auto GetSerializedInstrumentType() const {
    return static_cast<orderbook::serialize::InstrumentTypeCode>(
        instrument_type_);
  }
  auto SetInstrumentType(const InstrumentType instrument_type) -> BaseData& {
    instrument_type_ = instrument_type;
    return *this;
  }

  auto GetLastPrice() -> Price { return last_price_; }
  auto& GetLastPrice() const { return last_price_; }
  auto SetLastPrice(const Price last_price) -> BaseData& {
    last_price_ = last_price;
    return *this;
  }

  auto GetOrderPrice() -> Price { return order_price_; }
  auto& GetOrderPrice() const { return order_price_; }
  auto SetOrderPrice(const Price order_price) -> BaseData& {
    order_price_ = order_price;
    return *this;
  }

  auto GetLastQuantity() -> Quantity { return last_quantity_; }
  auto& GetLastQuantity() const { return last_quantity_; }
  auto SetLastQuantity(const Quantity last_quantity) -> BaseData& {
    last_quantity_ = last_quantity;
    return *this;
  }

  auto GetOrderQuantity() -> Quantity { return order_quantity_; }
  auto& GetOrderQuantity() const { return order_quantity_; }
  auto SetOrderQuantity(const Quantity order_quantity) -> BaseData& {
    order_quantity_ = order_quantity;
    return *this;
  }

  auto GetLeavesQuantity() -> Quantity { return leaves_quantity_; }
  auto& GetLeavesQuantity() const { return leaves_quantity_; }
  auto SetLeavesQuantity(const Quantity leaves_quantity) -> BaseData& {
    leaves_quantity_ = leaves_quantity;
    return *this;
  }

  auto GetExecutedQuantity() -> Quantity { return executed_quantity_; }
  auto& GetExecutedQuantity() const { return executed_quantity_; }
  auto SetExecutedQuantity(const Quantity executed_quantity) -> BaseData& {
    executed_quantity_ = executed_quantity;
    return *this;
  }

  auto GetExecutedValue() -> Quantity { return executed_value_; }
  auto& GetExecutedValue() const { return executed_value_; }
  auto SetExecutedValue(const ExecutedValue executed_value) -> BaseData& {
    executed_value_ = executed_value;
    return *this;
  }

  auto GetExecutionId() -> ExecutionId { return execution_id_; }
  auto& GetExecutionId() const { return execution_id_; }
  auto SetExecutionId(const ExecutionId execution_id) -> BaseData& {
    execution_id_ = execution_id;
    return *this;
  }

  auto GetAccountId() -> AccountId { return account_id_; }
  auto& GetAccountId() const { return account_id_; }
  auto SetAccountId(const AccountId account_id) -> BaseData& {
    account_id_ = account_id;
    return *this;
  }

  auto GetOrderId() -> OrderId { return order_id_; }
  auto& GetOrderId() const { return order_id_; }
  auto SetOrderId(const OrderId order_id) -> BaseData& {
    order_id_ = order_id;
    return *this;
  }

  auto GetQuoteId() -> QuoteId { return quote_id_; }
  auto& GetQuoteId() const { return quote_id_; }
  auto SetQuoteId(const QuoteId quote_id) -> BaseData& {
    quote_id_ = quote_id;
    return *this;
  }

  auto GetRoutingId() -> RoutingId { return routing_id_; }
  auto& GetRoutingId() const { return routing_id_; }
  auto SetRoutingId(const RoutingId routing_id) -> BaseData& {
    routing_id_ = routing_id;
    return *this;
  }

  auto GetClientOrderId() -> ClientOrderId { return client_order_id_; }
  auto& GetClientOrderId() const { return client_order_id_; }
  auto SetClientOrderId(const ClientOrderId& client_order_id) -> BaseData& {
    client_order_id_ = client_order_id;
    return *this;
  }

  auto GetOrigClientOrderId() -> OrigClientOrderId {
    return orig_client_order_id_;
  }
  auto& GetOrigClientOrderId() const { return orig_client_order_id_; }
  auto SetOrigClientOrderId(const OrigClientOrderId& orig_client_order_id)
      -> BaseData& {
    orig_client_order_id_ = orig_client_order_id;
    return *this;
  }

  auto GetSessionId() -> SessionId { return session_id_; }
  auto& GetSessionId() const { return session_id_; }
  auto SetSessionId(const SessionId session_id) -> BaseData& {
    session_id_ = session_id;
    return *this;
  }

  auto GetInstrumentId() -> InstrumentId { return instrument_id_; }
  auto& GetInstrumentId() const { return instrument_id_; }
  auto SetInstrumentId(const InstrumentId instrument_id) -> BaseData& {
    instrument_id_ = instrument_id;
    return *this;
  }

 protected:
  BaseData() : BaseData(0) {}

  BaseData(const TransactionId& tx_id)
      : transaction_id_(tx_id),
        create_tm_(TimeUtil::EpochNanos()),
        last_modify_tm_(TimeUtil::EpochNanos()),
        client_order_id_{},
        orig_client_order_id_{} {}

  BaseData(const TransactionId& tx_id, const RoutingId& routing_id,
           const Side& side, const OrderStatus& order_status,
           const TimeInForce& time_in_force, const OrderType& order_type,
           const ExecutionType& execution_type,
           const InstrumentType& instrument_type, const Price& last_price,
           const Price& order_price, const Quantity& last_quantity,
           const Quantity& order_quantity, const Quantity& leaves_quantity,
           const Quantity& executed_quantity,
           const ExecutedValue& executed_value, const ExecutionId& execution_id,
           const AccountId& account_id, const OrderId& order_id,
           const QuoteId& quote_id, const SessionId& session_id,
           const InstrumentId& instrument_id,
           const ClientOrderId& client_order_id,
           const OrigClientOrderId& orig_client_order_id)
      : transaction_id_(tx_id),
        create_tm_(TimeUtil::EpochNanos()),
        last_modify_tm_(TimeUtil::EpochNanos()),
        routing_id_(routing_id),
        side_(side),
        order_status_(order_status),
        time_in_force_(time_in_force),
        order_type_(order_type),
        execution_type_(execution_type),
        instrument_type_(instrument_type),
        last_price_(last_price),
        order_price_(order_price),
        last_quantity_(last_quantity),
        order_quantity_(order_quantity),
        leaves_quantity_(leaves_quantity),
        executed_quantity_(executed_quantity),
        executed_value_(executed_value),
        execution_id_(execution_id),
        account_id_(account_id),
        order_id_(order_id),
        quote_id_(quote_id),
        session_id_(session_id),
        instrument_id_(instrument_id),
        client_order_id_(client_order_id),
        orig_client_order_id_(orig_client_order_id) {}

  BaseData(const TransactionId& tx_id, const RoutingId& routing_id,
           const OrderStatus& order_status, const OrderId& order_id,
           const ClientOrderId& client_order_id,
           const OrigClientOrderId& orig_client_order_id)
      : transaction_id_(tx_id),
        create_tm_(TimeUtil::EpochNanos()),
        last_modify_tm_(TimeUtil::EpochNanos()),
        routing_id_(routing_id),
        order_status_(order_status),
        order_id_(order_id),
        client_order_id_(client_order_id),
        orig_client_order_id_(orig_client_order_id) {}

  auto SetTransactionId(const TransactionId tx_id) -> void {
    transaction_id_ = tx_id;
  }
  auto SetCreateTime(const Timestamp ts) -> void { create_tm_ = ts; }
  auto SetLastModifyTime(const Timestamp ts) -> void { last_modify_tm_ = ts; }

  TransactionId transaction_id_;
  Timestamp create_tm_;
  Timestamp last_modify_tm_;

  RoutingId routing_id_;
  Side side_;
  OrderStatus order_status_;
  TimeInForce time_in_force_;
  OrderType order_type_;
  ExecutionType execution_type_;
  InstrumentType instrument_type_;

  Price last_price_;
  Price order_price_;
  Quantity last_quantity_;
  Quantity order_quantity_;
  Quantity leaves_quantity_;
  Quantity executed_quantity_;
  ExecutedValue executed_value_;

  ExecutionId execution_id_;
  AccountId account_id_;
  OrderId order_id_;
  QuoteId quote_id_;
  SessionId session_id_;
  InstrumentId instrument_id_;
  ClientOrderId client_order_id_;
  OrigClientOrderId orig_client_order_id_;
};

}  // namespace orderbook::data
