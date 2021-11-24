#pragma once

#include "orderbook/data/data_types.h"

/**
 * The new order message type is used by institutions wishing to electronically
 * submit securities and forex orders to a broker for execution.
 */

namespace orderbook::data {
struct NewOrderSingle : BaseData {
  NewOrderSingle() : BaseData() {}

  NewOrderSingle(const orderbook::serialize::NewOrderSingle* table)
      : BaseData() {
    SetSide(static_cast<SideCode>(table->side()));
    SetOrderStatus(static_cast<OrderStatusCode>(table->order_status()));
    SetTimeInForce(static_cast<TimeInForceCode>(table->time_in_force()));
    SetOrderType(static_cast<OrderTypeCode>(table->order_type()));
    SetOrderPrice(table->order_price());
    SetOrderQuantity(table->order_quantity());
    SetAccountId(table->account_id());
    SetSessionId(table->session_id());
    SetInstrumentId(table->instrument_id());
    SetClientOrderId(table->client_order_id()->string_view());
  }

  auto SerializeTo(flatbuffers::FlatBufferBuilder& builder) const
      -> flatbuffers::Offset<orderbook::serialize::NewOrderSingle> {
    return orderbook::serialize::CreateNewOrderSingle(
        builder, GetSerializedSide(), GetSerializedOrderStatus(),
        GetSerializedTimeInForce(), GetSerializedOrderType(), GetOrderPrice(),
        GetOrderQuantity(), GetAccountId(), GetSessionId(), GetInstrumentId(),
        builder.CreateString(GetClientOrderId()));
  }
};
}  // namespace orderbook::data
