#pragma once

#include "orderbook/data/data_types.h"

/**
 * Order Cancel/Replace Request <G> (a.k.a. Order Modification Request) is used
 * to change the parameters of an existing order.
 *
 * Do not use this message to cancel the remaining quantity of an outstanding
 * order, use the Cancel Request <F> message for this purpose.
 *
 * Cancel/Replace will be used to change any valid attribute of an open order
 * (i.e. reduce/increase quantity, change limit price, change instructions,
 * etc.) It can be used to re-open a filled order by increasing OrderQty <38>.
 *
 * An immediate response to this message is required. It is recommended that an
 * ExecutionRpt<8> with ExecType <150>='Pending Replace' be sent unless the
 * Order Cancel/Replace Request <G> can be immediately accepted (ExecutionRpt<8>
 * with ExecType <150>='Replaced') or rejected (Order Cancel Reject <9>
 * message).
 *
 * The Cancel/Replace request will only be accepted if the order can
 * successfully be pulled back from the exchange floor without executing.
 * Requests which cannot be processed will be rejected using the Cancel Reject
 * <9> message. The Cancel Reject <9> message should provide the ClOrdID <11>
 * and OrigClOrdID <41> values which were specified on the Cancel/Replace
 * Request message for identification.
 *
 * Note that while it is necessary for the ClOrdID <11> to change and be unique,
 * the broker's OrderID <37> field does not necessarily have to change as a
 * result of the Cancel/Replace request.
 */

namespace orderbook::data {
struct OrderCancelReplaceRequest : BaseData {
  OrderCancelReplaceRequest() : BaseData() {}

  OrderCancelReplaceRequest(
      const orderbook::serialize::OrderCancelReplaceRequest* table)
      : BaseData() {
    SetSide(static_cast<SideCode>(table->side()));
    SetOrderType(static_cast<OrderTypeCode>(table->order_type()));
    SetOrderPrice(table->order_price());
    SetOrderQuantity(table->order_quantity());
    SetOrderId(table->order_id());
    SetSessionId(table->session_id());
    SetInstrumentId(table->instrument_id());
    SetClientOrderId(table->client_order_id()->str());
    SetOrigClientOrderId(table->orig_client_order_id()->str());
  }

  auto SerializeTo(flatbuffers::FlatBufferBuilder& builder) const
      -> flatbuffers::Offset<orderbook::serialize::OrderCancelReplaceRequest> {
    return orderbook::serialize::CreateOrderCancelReplaceRequest(
        builder, GetSerializedSide(), GetSerializedOrderType(), GetOrderPrice(),
        GetOrderQuantity(), GetOrderId(), GetSessionId(), GetInstrumentId(),
        builder.CreateString(GetClientOrderId()),
        builder.CreateString(GetOrigClientOrderId()));
  }
};
}  // namespace orderbook::data
