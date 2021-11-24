#pragma once

#include "orderbook/data/data_types.h"

/**
 * The Order Cancel Reject <9> message is issued by the broker upon receipt
 * of a Order Cancel Request <F> or Order Cancel/Replace Request <G> message
 * which cannot be honored. Requests to change price or decrease quantity are
 * executed only when an outstanding quantity exists. Filled orders cannot be
 * changed (i.e quantity reduced or price change. However, the broker/sellside
 * may support increasing the order quantity on a currently filled order).
 *
 * When rejecting a Cancel/Replace Request <G>, the Cancel Reject <9> message
 * should provide the ClOrdID <11> and OrigClOrdID <41> values which were
 * specified on the Cancel/Replace Request message for identification.
 *
 * The Execution <8> message responds to accepted Cancel Request <F> and
 * Cancel/Replace Request <G> messages.
 */

namespace orderbook::data {
struct OrderCancelReject : public BaseData {
 public:
  OrderCancelReject(const orderbook::serialize::OrderCancelReject* table)
      : BaseData() {
    SetOrderId(table->order_id());
    SetOrderStatus(static_cast<OrderStatusCode>(table->order_status()));
    cxl_rej_response_to_ =
        static_cast<CxlRejResponseToCode>(table->cxl_rej_response_to());
    SetClientOrderId(table->client_order_id()->string_view());
    SetOrigClientOrderId(table->orig_client_order_id()->string_view());
  }

  template <typename CancelRequest>
  OrderCancelReject(const TransactionId& tx_id,
                    const CancelRequest& cancel_request,
                    const CxlRejResponseTo& cxl_rej_response_to)
      : BaseData(tx_id, cancel_request.GetRoutingId(),
                 cancel_request.GetOrderStatus(), cancel_request.GetOrderId(),
                 cancel_request.GetClientOrderId(),
                 cancel_request.GetOrigClientOrderId()),
        cxl_rej_response_to_(cxl_rej_response_to) {}

  auto SerializeTo(flatbuffers::FlatBufferBuilder& builder) const
      -> flatbuffers::Offset<orderbook::serialize::OrderCancelReject> {
    return orderbook::serialize::CreateOrderCancelReject(
        builder, GetOrderId(), GetSerializedOrderStatus(),
        GetSerializedCxlRejResponseTo(), GetSessionId(),
        builder.CreateString(GetClientOrderId()),
        builder.CreateString(GetOrigClientOrderId()));
  }

  auto GetSerializedCxlRejResponseTo() const
      -> orderbook::serialize::CxlRejResponseToCode {
    return static_cast<orderbook::serialize::CxlRejResponseToCode>(
        cxl_rej_response_to_);
  }

  CxlRejResponseTo cxl_rej_response_to_;
};
}  // namespace orderbook::data
