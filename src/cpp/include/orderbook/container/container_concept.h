#pragma once

#include "orderbook/data/new_order_single.h"
#include "orderbook/data/order_cancel_replace_request.h"
#include "orderbook/data/order_cancel_request.h"

namespace orderbook::container {

// clang-format off
template <typename ContainerT, typename OrderT>
concept ContainerConcept = requires(ContainerT c,
                                    OrderT o,
                                    orderbook::data::OrderId id,
                                    orderbook::data::NewOrderSingle nos,
                                    orderbook::data::OrderCancelReplaceRequest ocrr,
                                    orderbook::data::OrderCancelRequest ocr)
{
  c.Add(nos, id);
  c.Modify(ocrr);
  c.Remove(ocr);
  c.Remove(o);
  c.Front();
  c.IsEmpty();
  c.Count();
  c.Clear();
};
// clang-format on
}  // namespace orderbook::container
