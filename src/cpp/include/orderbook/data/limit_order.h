#pragma once

#include "orderbook/data/data_types.h"

/**
 * This is the object that represents resting orders, stored inside the
 * limit order book.
 */

namespace orderbook::data {
struct LimitOrder : BaseData {
  LimitOrder() : BaseData() {}

  auto operator<(const LimitOrder& o) const -> bool {
    return order_price_ < o.order_price_;
  }

  auto operator>(const LimitOrder& o) const -> bool {
    return order_price_ > o.order_price_;
  }
};
}  // namespace orderbook::data
