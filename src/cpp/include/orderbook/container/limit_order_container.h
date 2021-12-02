#pragma once

#include <sstream>
#include <unordered_set>

#include "boost/functional/hash.hpp"
#include "orderbook/data/data_types.h"
#include "orderbook/data/new_order_single.h"
#include "orderbook/data/order_cancel_request.h"

namespace orderbook::container {

template <typename Key, typename Order, typename List, typename Compare>
struct LimitOrderContainer {
  using OrderId = orderbook::data::OrderId;
  using SessionId = orderbook::data::SessionId;
  using ClientOrderId = orderbook::data::ClientOrderId;
  using OrderStatus = orderbook::data::OrderStatus;
  using ReturnPair = std::pair<bool, Order>;
  using Iterator = typename List::iterator;
  using PriceLevelMap = std::map<Key, List, Compare>;
  using OrderIdMap = std::unordered_map<OrderId, Iterator>;
  using ClientOrderIdKey = std::pair<SessionId, ClientOrderId>;
  using ClientOrderIdMap = std::unordered_map<ClientOrderIdKey, OrderId,
                                              boost::hash<ClientOrderIdKey>>;
  using NewOrderSingle = orderbook::data::NewOrderSingle;
  using OrderCancelRequest = orderbook::data::OrderCancelRequest;
  using OrderCancelReplaceRequest = orderbook::data::OrderCancelReplaceRequest;

  auto IsEmpty() -> bool { return size_ == 0; }
  auto Count() const -> std::size_t { return size_; }

  /**
   * Takes the modify request and resting order and updates the necessary data
   * structures to reflect the new client order state.
   */
  auto UpdateClientOrderId(const OrderCancelReplaceRequest& modify_request,
                           Order& order) -> void {
    // Erase the old key
    clord_id_map_.erase(
        {modify_request.GetSessionId(), modify_request.GetOrigClientOrderId()});

    // Add the new key
    const ClientOrderIdKey& new_key = {modify_request.GetSessionId(),
                                       modify_request.GetClientOrderId()};
    clord_id_map_.emplace(new_key, modify_request.GetSessionId());

    // Update the order
    order.SetClientOrderId(modify_request.GetClientOrderId())
        .SetOrigClientOrderId(modify_request.GetOrigClientOrderId());
  }

  /**
   * Adds the order into the order book w/o checking for valid state.
   */
  auto AddDirect(Order& order) -> void {
    auto& list = price_level_map_[order.GetOrderPrice()];
    auto&& iter = list.insert(list.end(), order);
    order_id_map_.emplace(order.GetOrderId(), std::forward<Iterator>(iter));
  }

  /**
   * Removes the order from the order book w/o checking for valid state.
   */
  auto RemoveDirect(const Order& order) -> void {
    const auto& order_id_map_iter = order_id_map_.find(order.GetOrderId());
    auto& list = price_level_map_[order.GetOrderPrice()];

    list.erase(order_id_map_iter->second);
    order_id_map_.erase(order_id_map_iter);
    --size_;

    if (list.empty()) {
      price_level_map_.erase(order.GetOrderPrice());
    }
  }

  OrderIdMap order_id_map_{};
  PriceLevelMap price_level_map_{};
  ClientOrderIdMap clord_id_map_{};
  std::size_t size_{0};
};

}  // namespace orderbook::container
