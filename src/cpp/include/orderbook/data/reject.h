#pragma once

#include "orderbook/data/data_types.h"

namespace orderbook::data {
struct Reject : BaseData {
  Reject(const ClientOrderId client_order_id,
         const OrigClientOrderId orig_client_order_id)
      : BaseData(),
        client_order_id_(client_order_id),
        orig_client_order_id_(orig_client_order_id) {}

  ClientOrderId client_order_id_;
  OrigClientOrderId orig_client_order_id_;
};
}  // namespace orderbook::data
