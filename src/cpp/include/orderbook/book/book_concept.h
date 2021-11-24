#pragma once

#include "orderbook/data/data_types.h"
#include "orderbook/data/new_order_single.h"
#include "orderbook/data/order_cancel_replace_request.h"
#include "orderbook/data/order_cancel_request.h"

namespace orderbook::book {

// clang-format off
template <typename BookT>
concept BookConcept = requires(BookT b,
                               orderbook::data::NewOrderSingle nos,
                               orderbook::data::OrderCancelRequest ocr,
                               orderbook::data::OrderCancelReplaceRequest ocrr,
                               orderbook::data::Side s) {
  b.Add(nos);
  b.Modify(ocrr);
  b.Cancel(ocr);
  b.Match(s);
  b.Empty();
  b.Reset();
};
// clang-format on
}  // namespace orderbook::book
