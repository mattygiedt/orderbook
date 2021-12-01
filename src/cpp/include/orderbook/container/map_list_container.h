#pragma once

#include <sstream>

#include "boost/intrusive_ptr.hpp"
#include "orderbook/data/data_types.h"
#include "orderbook/data/order_cancel_request.h"

namespace orderbook::container {

template <typename Key, typename Order, typename Compare>
class MapListContainer {
 private:
  using OrderId = orderbook::data::OrderId;
  using OrderPtr = boost::intrusive_ptr<Order>;
  using List = std::list<OrderPtr>;
  using Iterator = typename std::list<OrderPtr>::iterator;
  using PriceLevelMap = std::map<Key, List, Compare>;
  using OrderMap = std::unordered_map<OrderId, Iterator>;
  using OrderCancelRequest = orderbook::data::OrderCancelRequest;
  using OrderCancelReplaceRequest = orderbook::data::OrderCancelReplaceRequest;

 public:
  /**
   * Add the order to the container.
   *
   * Returns true if the order was successfully added,
   * false otherwise.
   */
  auto Add(OrderPtr order) -> bool {
    const auto& order_id = order->GetOrderId();

    if (!order_map_.contains(order_id)) {
      auto& list = price_level_map_[order->GetOrderPrice()];
      auto&& iter = list.insert(list.end(), std::move(order));
      order_map_.emplace(order_id, std::forward<Iterator>(iter));
      ++size_;
      return true;
    }

    return false;
  }

  /**
   * Attempts to modify an existing order.
   *
   * Returns std::pair[true, resting_order] if the order was found and
   * successfully modified, std::pair[false, nullptr] if not.
   */
  auto Modify(const OrderCancelReplaceRequest& modify_request)
      -> std::pair<bool, OrderPtr> {
    auto order_map_iter = order_map_.find(modify_request.GetOrderId());
    if (order_map_iter != order_map_.end()) {
      auto& list = price_level_map_[modify_request.GetOrderPrice()];
      auto& iter = order_map_iter->second;
      auto order = *iter;

      // Ensure previous clord_id matches current clord_id, and that the
      // new order quantity is greater-than-or-equals the current executed
      // quantity.
      if (order->GetClientOrderId() == modify_request.GetOrigClientOrderId() &&
          order->GetExecutedQuantity() <= modify_request.GetOrderQuantity()) {
        // Identify what has changed
        const bool prc_changed =
            order->GetOrderPrice() != modify_request.GetOrderPrice();
        const bool qty_changed =
            order->GetOrderQuantity() != modify_request.GetOrderQuantity();

        if (prc_changed) {
          // Change in price requires remove + add
          auto&& remove_pair = Remove(*order);
          assert(remove_pair.first == true);
          order->SetClientOrderId(modify_request.GetClientOrderId())
              .SetOrigClientOrderId(modify_request.GetOrigClientOrderId())
              .SetOrderQuantity(modify_request.GetOrderQuantity())
              .SetOrderPrice(modify_request.GetOrderPrice())
              .UpdateOrderStatus()
              .Mark();
          auto add_flag = Add(order);
          assert(add_flag == true);
        } else if (qty_changed) {
          assert(prc_changed == false);
          if (order->GetOrderQuantity() > modify_request.GetOrderQuantity()) {
            // Decrease of order quantity maintains queue spot priority
            order->SetClientOrderId(modify_request.GetClientOrderId())
                .SetOrigClientOrderId(modify_request.GetOrigClientOrderId())
                .SetOrderQuantity(modify_request.GetOrderQuantity())
                .UpdateOrderStatus()
                .Mark();
          } else {
            // Update the order quantity, and move it to the end of the queue
            order->SetClientOrderId(modify_request.GetClientOrderId())
                .SetOrigClientOrderId(modify_request.GetOrigClientOrderId())
                .SetOrderQuantity(modify_request.GetOrderQuantity())
                .UpdateOrderStatus()
                .Mark();

            list.splice(list.end(), list, iter);
          }
        } else {
          // Currently a NOP, will revisit when we have a strategy for TIF
          order->SetClientOrderId(modify_request.GetClientOrderId())
              .SetOrigClientOrderId(modify_request.GetOrigClientOrderId())
              .Mark();
        }

        return std::make_pair(true, std::move(order));
      }
    }

    return std::make_pair(false, nullptr);
  }

  /**
   * Remove the order from the container.
   *
   * Returns std::pair[true, resting_order] if the order was found and erased,
   * std::pair[false, nullptr] if not found.
   */
  template <typename CancelRequest>
  auto Remove(const CancelRequest& cancel_request)
      -> std::pair<bool, OrderPtr> {
    auto order_map_iter = order_map_.find(cancel_request.GetOrderId());
    if (order_map_iter != order_map_.end()) {
      auto& list = price_level_map_[cancel_request.GetOrderPrice()];
      auto& iter = order_map_iter->second;
      auto& order = *iter;

      // TODO: validate the order the iterator is pointing to has the same
      // attributes as the cance_request message
      list.erase(iter);
      order_map_.erase(order_map_iter);
      --size_;

      if (list.empty()) {
        price_level_map_.erase(cancel_request.GetOrderPrice());
      }

      return std::make_pair(true, order);
    }

    return std::make_pair(false, nullptr);
  }

  /**
   * Returns the first order in the list that is mapped to the first
   * key in the price_level_map.
   */
  auto Front() -> OrderPtr& { return price_level_map_.begin()->second.front(); }

  auto IsEmpty() -> bool { return size_ == 0; }
  auto Count() const -> std::size_t { return size_; }
  auto Clear() -> void {
    price_level_map_.clear();
    order_map_.clear();
    size_ = 0;
  }

  auto DebugString() -> std::string {
    std::stringstream ss;

    for (auto const& [key, list] : price_level_map_) {
      ss << key << std::endl;
      for (auto& order : list) {
        ss << " " << order->GetOrderId() << " "
           << std::string(order->GetClientOrderId()) << " "
           << order->GetOrderPrice() << " " << order->GetOrderQuantity()
           << std::endl;
      }
    }

    return ss.str();
  }

 private:
  PriceLevelMap price_level_map_;
  OrderMap order_map_;
  std::size_t size_{0};
};
}  // namespace orderbook::container
