#pragma once

#include "orderbook/data/data_types.h"
#include "orderbook/data/empty.h"
#include "orderbook/data/event_types.h"
#include "orderbook/data/new_order_single.h"
#include "orderbook/data/order_cancel_replace_request.h"
#include "orderbook/data/order_cancel_request.h"
#include "spdlog/spdlog.h"

namespace orderbook::book {

using namespace orderbook::data;

template <typename BidContainerType, typename AskContainerType,
          typename OrderType, typename EventDispatcher>
class LimitOrderBook {
 private:
  using Order = OrderType;
  using EmptyType = orderbook::data::Empty;

 public:
  LimitOrderBook(std::shared_ptr<EventDispatcher> dispatcher)
      : dispatcher_(std::move(dispatcher)), data_{EmptyType()} {}

  auto Add(const NewOrderSingle& new_order_single) -> void {
    Order order;
    order.SetOrderId(++order_id_)
        .SetRoutingId(new_order_single.GetRoutingId())
        .SetSessionId(new_order_single.GetSessionId())
        .SetAccountId(new_order_single.GetAccountId())
        .SetInstrumentId(new_order_single.GetInstrumentId())
        .SetOrderType(new_order_single.GetOrderType())
        .SetOrderPrice(new_order_single.GetOrderPrice())
        .SetOrderQuantity(new_order_single.GetOrderQuantity())
        .SetLeavesQuantity(new_order_single.GetOrderQuantity())
        .SetSide(new_order_single.GetSide())
        .SetTimeInForce(new_order_single.GetTimeInForce())
        .SetOrderStatus(OrderStatus::kPendingNew)
        .SetExecutedQuantity(0)
        .SetLastPrice(0)
        .SetLastQuantity(0)
        .SetExecutedValue(0);

    DispatchOrderStatus(EventType::kOrderPendingNew, order);

    if (order.IsBuyOrder()) {
      auto&& [added, added_order] = bids_.Add(order);
      if (added) {
        // Order was accepted, and added to book
        added_order.SetOrderStatus(OrderStatus::kNew);
        DispatchOrderStatus(EventType::kOrderNew, added_order);
        Match(SideCode::kBuy);
      } else {
        // Order was rejected, and not added to book
        order.SetOrderStatus(OrderStatus::kRejected);
        DispatchOrderStatus(EventType::kOrderRejected, order);
        return;
      }

    } else {
      auto&& [added, added_order] = asks_.Add(order);
      if (added) {
        // Order was accepted, and added to book
        added_order.SetOrderStatus(OrderStatus::kNew);
        DispatchOrderStatus(EventType::kOrderNew, added_order);
        Match(SideCode::kSell);
      } else {
        // Order was rejected, and not added to book
        order.SetOrderStatus(OrderStatus::kRejected);
        DispatchOrderStatus(EventType::kOrderRejected, order);
        return;
      }
    }
  }

  /**
   * For the time being, deny any attempt to modify resting orders.
   */
  auto Modify(const OrderCancelReplaceRequest& cancel_replace_request) -> void {
    CancelRejectOrder(cancel_replace_request,
                      CxlRejResponseTo::kOrderCancelReplaceRequest);
  }

  /**
   * Try to cancel an order that we think is resting on the book.
   */
  auto Cancel(const OrderCancelRequest& cancel_request) -> void {
    if (cancel_request.IsBuyOrder()) {
      auto&& [removed, removed_order] = bids_.Remove(cancel_request);

      if (removed) {
        CancelOrder(removed_order);
      } else {
        CancelRejectOrder(cancel_request,
                          CxlRejResponseTo::kOrderCancelRequest);
      }
    } else {
      auto&& [removed, removed_order] = asks_.Remove(cancel_request);

      if (removed) {
        CancelOrder(removed_order);
      } else {
        CancelRejectOrder(cancel_request,
                          CxlRejResponseTo::kOrderCancelRequest);
      }
    }
  }

  auto Match(const SideCode aggressor) -> void {
    while (!bids_.IsEmpty() && !asks_.IsEmpty()) {
      auto& bid = bids_.Front();
      auto& ask = asks_.Front();

      if (bid.GetOrderPrice() >= ask.GetOrderPrice()) {
        // Calculate the execution price
        Price prc;
        if (aggressor == SideCode::kBuy) {
          prc = ask.GetOrderPrice();
        } else {
          prc = bid.GetOrderPrice();
        }

        if (bid.GetLeavesQuantity() == ask.GetLeavesQuantity()) {
          const auto qty = bid.GetLeavesQuantity();

          // fully execute bid
          ExecuteOrder(bid, prc, qty);
          bids_.Remove(bid);

          // fully execute ask
          ExecuteOrder(ask, prc, qty);
          asks_.Remove(ask);

        } else if (bid.GetLeavesQuantity() < ask.GetLeavesQuantity()) {
          const auto qty = bid.GetLeavesQuantity();

          // fully execute bid
          ExecuteOrder(bid, prc, qty);
          bids_.Remove(bid);

          // partially execute ask
          ExecuteOrder(ask, prc, qty);

        } else if (bid.GetLeavesQuantity() > ask.GetLeavesQuantity()) {
          const auto qty = ask.GetLeavesQuantity();

          // fully execute ask
          ExecuteOrder(ask, prc, qty);
          asks_.Remove(ask);

          // partially execute bid
          ExecuteOrder(bid, prc, qty);
        }
      } else {
        return;
      }
    }
  }

  /**
   * Returns true iff both order containers are empty.
   */
  auto Empty() -> bool { return (bids_.IsEmpty() && asks_.IsEmpty()); }

  /**
   * Clears the order containers.
   */
  auto Reset() -> void {
    bids_.Clear();
    asks_.Clear();
  }

 private:
  auto ExecuteOrder(Order& order, const Price& prc, const Quantity& qty)
      -> void {
    order.SetExecutedQuantity(order.GetExecutedQuantity() + qty)
        .SetLeavesQuantity(order.GetLeavesQuantity() - qty)
        .SetLastPrice(prc)
        .SetLastQuantity(qty)
        .SetExecutedValue(order.GetExecutedValue() + (prc * qty));

    if (order.GetLeavesQuantity() > 0) {
      order.SetOrderStatus(OrderStatus::kPartiallyFilled);
      DispatchOrderExecution(EventType::kOrderPartiallyFilled, order);
    } else {
      order.SetOrderStatus(OrderStatus::kFilled);
      DispatchOrderExecution(EventType::kOrderFilled, order);
    }
  }

  auto CancelOrder(Order& order) -> void {
    order.SetLastPrice(0)
        .SetLastQuantity(0)
        .SetLeavesQuantity(0)
        .SetOrderQuantity(order.GetExecutedQuantity())
        .SetOrderStatus(OrderStatus::kCancelled);
    DispatchOrderStatus(EventType::kOrderCancelled, order);
  }

  /**
   * Send a cancel reject message back to the client
   */
  template <typename CancelRequest>
  auto CancelRejectOrder(const CancelRequest& cancel_request,
                         const CxlRejResponseTo& cxl_rej_response_to) -> void {
    data_ = OrderCancelReject(++tx_id_, cancel_request, cxl_rej_response_to);
    dispatcher_->dispatch(EventType::kOrderCancelRejected, data_);
  }

  /**
   * Used to communicate order status back to client.
   */
  auto DispatchOrderStatus(const EventType& event_type, const Order& order)
      -> void {
    data_ = ExecutionReport(++tx_id_, ++exec_id_, order);
    dispatcher_->dispatch(event_type, data_);
  }

  /**
   * Used to communicate order executions back to client.
   */
  auto DispatchOrderExecution(const EventType& event_type, const Order& order)
      -> void {
    data_ = ExecutionReport(++tx_id_, ++exec_id_, order);
    dispatcher_->dispatch(event_type, data_);
  }

  TransactionId tx_id_{0};
  ExecutionId exec_id_{0};
  OrderId order_id_{0};

  std::shared_ptr<EventDispatcher> dispatcher_;
  EventData data_;

  BidContainerType bids_;
  AskContainerType asks_;
};
}  // namespace orderbook::book
