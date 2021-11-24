#pragma once

#include "orderbook/data/data_types.h"

/**
 * The Execution Report <8> message is used to:
 *  1. confirm the receipt of an order
 *  2. confirm changes to an existing order (i.e. accept cancel and replace
 *     requests)
 *  3. relay order status information
 *  4. relay fill information on working orders
 *  5. reject orders
 *  6. report post-trade fees calculations associated with a trade
 */

namespace orderbook::data {
struct ExecutionReport : BaseData {
  // template <typename Order>
  // ExecutionReport(const TransactionId& tx_id, const Order& order,
  //                const OrderStatus& order_status, const ExecutionId& exec_id)
  //    : BaseData(tx_id, order.GetRoutingId(), order.GetSide(), order_status,
  //               order.GetTimeInForce(), order.GetOrderType(),
  //               ExecutionType::kNew, order.GetInstrumentType(),
  //               order.GetLastPrice(), order.GetOrderPrice(),
  //               order.GetLastQuantity(), order.GetOrderQuantity(),
  //               order.GetLeavesQuantity(), order.GetExecutedQuantity(),
  //               order.GetExecutedValue(), exec_id, order.GetAccountId(),
  //               order.GetOrderId(), order.GetQuoteId(), order.GetSessionId(),
  //               order.GetInstrumentId(), order.GetClientOrderId(),
  //               order.GetOrigClientOrderId()) {}

  template <typename Order>
  ExecutionReport(const TransactionId& tx_id, const ExecutionId& exec_id,
                  const Order& order)
      : BaseData(tx_id, order.GetRoutingId(), order.GetSide(),
                 order.GetOrderStatus(), order.GetTimeInForce(),
                 order.GetOrderType(), ExecutionType::kNew,
                 order.GetInstrumentType(), order.GetLastPrice(),
                 order.GetOrderPrice(), order.GetLastQuantity(),
                 order.GetOrderQuantity(), order.GetLeavesQuantity(),
                 order.GetExecutedQuantity(), order.GetExecutedValue(), exec_id,
                 order.GetAccountId(), order.GetOrderId(), order.GetQuoteId(),
                 order.GetSessionId(), order.GetInstrumentId(),
                 order.GetClientOrderId(), order.GetOrigClientOrderId()) {}

  ExecutionReport(const orderbook::serialize::ExecutionReport* table)
      : BaseData() {
    SetSide(static_cast<SideCode>(table->side()));
    SetOrderStatus(static_cast<OrderStatusCode>(table->order_status()));
    SetTimeInForce(static_cast<TimeInForceCode>(table->time_in_force()));
    SetOrderType(static_cast<OrderTypeCode>(table->order_type()));
    SetExecutionType(static_cast<ExecutionTypeCode>(table->execution_type()));
    SetLastPrice(table->last_price());
    SetLastQuantity(table->last_quantity());
    SetOrderPrice(table->order_price());
    SetOrderQuantity(table->order_quantity());
    SetLeavesQuantity(table->leaves_quantity());
    SetExecutedValue(table->executed_value());
    SetExecutionId(table->execution_id());
    SetAccountId(table->account_id());
    SetOrderId(table->order_id());
    SetQuoteId(table->quote_id());
    SetSessionId(table->session_id());
    SetInstrumentId(table->instrument_id());
    SetClientOrderId(table->client_order_id()->string_view());
    SetOrigClientOrderId(table->orig_client_order_id()->string_view());
  }

  auto SerializeTo(flatbuffers::FlatBufferBuilder& builder) const
      -> flatbuffers::Offset<orderbook::serialize::ExecutionReport> {
    return orderbook::serialize::CreateExecutionReport(
        builder, GetSerializedSide(), GetSerializedOrderStatus(),
        GetSerializedTimeInForce(), GetSerializedOrderType(),
        GetSerializedExecutionType(), GetLastPrice(), GetLastQuantity(),
        GetOrderPrice(), GetOrderQuantity(), GetLeavesQuantity(),
        GetExecutedValue(), GetExecutionId(), GetAccountId(), GetOrderId(),
        GetQuoteId(), GetSessionId(), GetInstrumentId(),
        builder.CreateString(GetClientOrderId()),
        builder.CreateString(GetOrigClientOrderId()));
  }
};
}  // namespace orderbook::data
