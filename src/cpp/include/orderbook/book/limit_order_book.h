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

  inline static OrderId order_id{0};

 public:
  LimitOrderBook(std::shared_ptr<EventDispatcher> dispatcher)
      : dispatcher_(std::move(dispatcher)), data_{EmptyType()} {}

  /**
   * Attempt to add a new order to the order book.
   */
  auto Add(const NewOrderSingle& add_request) -> void {
    if (add_request.IsBuyOrder()) {
      // object pool is empty
      if (bids_.Available() == 0) {
        spdlog::error("bids_.Available() == 0");
        DispatchOrderStatus(EventType::kOrderRejected, add_request);
        return;
      }

      auto&& [added, order] = bids_.Add(add_request, ++order_id);

      if (added) {
        // Order was accepted, and added to book
        DispatchOrderStatus(EventType::kOrderNew, order);
        Match(SideCode::kBuy);
      } else {
        // Order was rejected, and not added to book
        DispatchOrderStatus(EventType::kOrderRejected, order);
        return;
      }

    } else {
      // object pool is empty
      if (asks_.Available() == 0) {
        spdlog::error("asks_.Available() == 0");
        DispatchOrderStatus(EventType::kOrderRejected, add_request);
        return;
      }

      auto&& [added, order] = asks_.Add(add_request, ++order_id);

      if (added) {
        // Order was accepted, and added to book
        DispatchOrderStatus(EventType::kOrderNew, order);
        Match(SideCode::kSell);
      } else {
        // Order was rejected, and not added to book
        DispatchOrderStatus(EventType::kOrderRejected, order);
        return;
      }
    }
  }

  /**
   * Attempt to modify a resting order.
   */
  auto Modify(const OrderCancelReplaceRequest& modify_request) -> void {
    if (modify_request.IsBuyOrder()) {
      auto&& [modified, modified_order] = bids_.Modify(modify_request);

      if (modified) {
        DispatchOrderStatus(EventType::kOrderModified, modified_order);
        Match(SideCode::kBuy);
      } else {
        CancelRejectOrder(modify_request,
                          CxlRejResponseTo::kOrderCancelReplaceRequest);
      }
    } else {
      auto&& [modified, modified_order] = asks_.Modify(modify_request);

      if (modified) {
        DispatchOrderStatus(EventType::kOrderModified, modified_order);
        Match(SideCode::kSell);
      } else {
        CancelRejectOrder(modify_request,
                          CxlRejResponseTo::kOrderCancelReplaceRequest);
      }
    }
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
    order.SetLeavesQuantity(order.GetLeavesQuantity() - qty)
        .SetExecutedQuantity(order.GetExecutedQuantity() + qty)
        .SetExecutedValue(order.GetExecutedValue() + (prc * qty))
        .SetLastPrice(prc)
        .SetLastQuantity(qty)
        .Mark();

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
        .SetOrderStatus(OrderStatus::kCancelled)
        .Mark();
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
  template <typename OrderData>
  auto DispatchOrderStatus(const EventType& event_type, const OrderData& order)
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

  std::shared_ptr<EventDispatcher> dispatcher_;
  EventData data_;

  BidContainerType bids_;
  AskContainerType asks_;
};
}  // namespace orderbook::book
