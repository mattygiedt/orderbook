#pragma once

#include "boost/intrusive_ptr.hpp"
#include "orderbook/data/order_cancel_request.h"

namespace orderbook::container {

// clang-format off
template <typename ContainerT, typename OrderT>
concept ContainerConcept = requires(ContainerT c,
                                    boost::intrusive_ptr<OrderT> o,
                                    orderbook::data::OrderCancelRequest ocr)
{
  c.Add(o);
  c.Remove(ocr);
  c.Remove(o);
  c.Front();
  c.IsEmpty();
  c.Count();
  c.Clear();
};
// clang-format on
}  // namespace orderbook::container
