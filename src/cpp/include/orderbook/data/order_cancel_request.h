#pragma once

#include "orderbook/data/data_types.h"

/**
 * The Order Cancel Request <F> message requests the cancelation of all of the
 * remaining quantity of an existing order. Note that the Order Cancel/Replace
 * Request <G> should be used to partially cancel (reduce) an order.
 *
 * The request will only be accepted if the order can successfully be pulled
 * back from the exchange floor without executing.
 */

namespace orderbook::data {
struct OrderCancelRequest : BaseData {
  OrderCancelRequest() : BaseData() {}

  OrderCancelRequest(const orderbook::serialize::OrderCancelRequest* table)
      : BaseData() {
    SetSide(static_cast<SideCode>(table->side()));
    SetOrderQuantity(table->order_quantity());
    SetOrderId(table->order_id());
    SetSessionId(table->session_id());
    SetInstrumentId(table->instrument_id());
    SetClientOrderId(table->client_order_id()->str());
    SetOrigClientOrderId(table->orig_client_order_id()->str());
  }

  auto SerializeTo(flatbuffers::FlatBufferBuilder& builder) const
      -> flatbuffers::Offset<orderbook::serialize::OrderCancelRequest> {
    return orderbook::serialize::CreateOrderCancelRequest(
        builder, GetSerializedSide(), GetOrderQuantity(), GetOrderId(),
        GetSessionId(), GetInstrumentId(),
        builder.CreateString(GetClientOrderId()),
        builder.CreateString(GetOrigClientOrderId()));
  }
};
}  // namespace orderbook::data
