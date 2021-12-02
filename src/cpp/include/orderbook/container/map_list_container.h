#pragma once

#include <sstream>
#include <unordered_set>

#include "boost/functional/hash.hpp"
#include "boost/intrusive_ptr.hpp"
#include "orderbook/data/data_types.h"
#include "orderbook/data/new_order_single.h"
#include "orderbook/data/order_cancel_request.h"

namespace orderbook::container {

template <typename Key, typename Order, typename Pool, typename Compare>
class MapListContainer {
 private:
  using OrderId = orderbook::data::OrderId;
  using SessionId = orderbook::data::SessionId;
  using ClientOrderId = orderbook::data::ClientOrderId;
  using OrderStatus = orderbook::data::OrderStatus;
  using OrderPtr = boost::intrusive_ptr<Order>;
  using ReturnPair = std::pair<bool, Order>;
  using List = std::list<OrderPtr>;
  using Iterator = typename List::iterator;
  using PriceLevelMap = std::map<Key, List, Compare>;
  using OrderIdMap = std::unordered_map<OrderId, Iterator>;
  using ClientOrderIdKey = std::pair<SessionId, ClientOrderId>;
  using ClientOrderIdMap = std::unordered_map<ClientOrderIdKey, OrderId,
                                              boost::hash<ClientOrderIdKey>>;
  using NewOrderSingle = orderbook::data::NewOrderSingle;
  using OrderCancelRequest = orderbook::data::OrderCancelRequest;
  using OrderCancelReplaceRequest = orderbook::data::OrderCancelReplaceRequest;

  inline static Pool& pool = Pool::Instance();
  inline static ReturnPair kFalsePair = {false, Order{}};

 public:
  /**
   * Add the new order single to the container.
   *
   * Returns true if the order was successfully added,
   * false otherwise.
   */
  auto Add(const NewOrderSingle& order_request, const OrderId& order_id)
      -> ReturnPair {
    // Create a new order intrusive_ptr
    auto order = MapListContainer::MakeOrder(order_request, order_id);

    // Does our clord_id set contain the requested client_order_id key?
    const ClientOrderIdKey& clord_id_key = {order_request.GetSessionId(),
                                            order_request.GetClientOrderId()};

    const auto& clord_id_map_iter = clord_id_map_.find(clord_id_key);

    if (clord_id_map_iter != clord_id_map_.end()) {
      spdlog::warn(
          "MapListContainer::Add duplicate clord_id '{}' for session {}, "
          "rejecting order_id: {}",
          clord_id_key.second, clord_id_key.first, order_id);
      order->SetOrderStatus(OrderStatus::kRejected);
      return {false, *order};
    }

    // Add the order to the order book
    order->SetOrderStatus(OrderStatus::kNew);
    auto& list = price_level_map_[order->GetOrderPrice()];
    auto&& iter = list.insert(list.end(), std::move(order));
    order_id_map_.emplace(order_id, std::forward<Iterator>(iter));
    clord_id_map_.emplace(clord_id_key, order_id);
    ++size_;

    return {true, *(*iter)};
  }

  /**
   * Attempts to modify an existing order.
   *
   * Returns std::pair[true, resting_order] if the order was found and
   * successfully modified, std::pair[false, empty_order] if not.
   */
  auto Modify(const OrderCancelReplaceRequest& modify_request) -> ReturnPair {
    // find the order by the order_id we assigned in MapListContainer::Add
    const auto& order_id_map_iter =
        order_id_map_.find(modify_request.GetOrderId());

    if (order_id_map_iter != order_id_map_.end()) {
      auto& list = price_level_map_[modify_request.GetOrderPrice()];
      auto& iter = order_id_map_iter->second;
      auto order = *iter;

      // Ensure previous clord_id matches current clord_id, and that the
      // new order quantity is greater-than-or-equals the current executed
      // quantity.
      if (order->GetSessionId() == modify_request.GetSessionId() &&
          order->GetClientOrderId() == modify_request.GetOrigClientOrderId() &&
          order->GetExecutedQuantity() <= modify_request.GetOrderQuantity()) {
        // Update the clord_id values in the order and our client order id set
        UpdateClientOrderId(modify_request, order);

        // Identify what has changed
        const bool prc_changed =
            order->GetOrderPrice() != modify_request.GetOrderPrice();
        const bool qty_changed =
            order->GetOrderQuantity() != modify_request.GetOrderQuantity();

        if (prc_changed) {
          // Change in price requires remove + update + add
          RemoveDirect(order);

          // Modify the order details
          order->SetOrderQuantity(modify_request.GetOrderQuantity())
              .SetOrderPrice(modify_request.GetOrderPrice())
              .UpdateOrderStatus()
              .Mark();

          // Add order back into order book
          AddDirect(order);

        } else if (qty_changed) {
          assert(prc_changed == false);
          if (order->GetOrderQuantity() > modify_request.GetOrderQuantity()) {
            // Decrease of order quantity maintains queue spot priority
            order->SetOrderQuantity(modify_request.GetOrderQuantity())
                .UpdateOrderStatus()
                .Mark();
          } else {
            // Update the order quantity, and move it to the end of the queue
            order->SetOrderQuantity(modify_request.GetOrderQuantity())
                .UpdateOrderStatus()
                .Mark();

            list.splice(list.end(), list, iter);
          }
        } else {
          // Currently a NOP, will revisit when we have a strategy for
          // additional fields other than price / quantity.
          order->Mark();
        }

        return {true, *order};
      }

      spdlog::warn(
          "MapListContainer::Modify business match reject order[ order_id {} ] "
          "-> [ sess: {}, clord_id: {}, orig_clord_id: {}], modify_request[ "
          "order_id {} ] -> [ sess: {}, clord_id: {}, orig_clord_id: {} ]",
          order->GetOrderId(), order->GetSessionId(), order->GetClientOrderId(),
          order->GetOrigClientOrderId(), modify_request.GetOrderId(),
          modify_request.GetSessionId(), modify_request.GetClientOrderId(),
          modify_request.GetOrigClientOrderId());

      return kFalsePair;
    }

    spdlog::warn(
        "MapListContainer::Modify unknown order_id: {} for modify_request: [ "
        "sess: {}, clord_id: {}, orig_clord_id: {} ]",
        modify_request.GetOrderId(), modify_request.GetSessionId(),
        modify_request.GetClientOrderId(),
        modify_request.GetOrigClientOrderId());

    return kFalsePair;
  }

  /**
   * Remove the order from the container.
   *
   * Returns std::pair[true, resting_order] if the order was found and erased,
   * std::pair[false, empty_order] if not found.
   */
  template <typename CancelRequest>
  auto Remove(const CancelRequest& cancel_request) -> ReturnPair {
    const auto& order_id_map_iter =
        order_id_map_.find(cancel_request.GetOrderId());

    const auto& clord_id_map_iter = clord_id_map_.find(
        {cancel_request.GetSessionId(), cancel_request.GetClientOrderId()});

    // boolean helpers to ensure valid book state if we can't find the order
    const bool found_order_id_map = order_id_map_iter != order_id_map_.end();
    const bool found_clord_id_map = clord_id_map_iter != clord_id_map_.end();

    // Ensure both order maps have the order
    if (found_order_id_map && found_clord_id_map) {
      // get the iterator pointing to this order
      auto& iter = order_id_map_iter->second;
      auto& order = *iter;

      // find the list at this price level
      auto& list = price_level_map_[cancel_request.GetOrderPrice()];

      // remove the order from the list and our maps
      list.erase(iter);
      order_id_map_.erase(order_id_map_iter);
      clord_id_map_.erase(clord_id_map_iter);

      // decrease the order count by one
      --size_;

      // if our price level is now empty, remove it, too
      if (list.empty()) {
        price_level_map_.erase(cancel_request.GetOrderPrice());
      }

      return {true, *order};
    }

    spdlog::warn(
        "MapListContainer::Remove unknown order for cancel_request: [ "
        "order_id: {}, sess: {}, clord_id: {}, orig_clord_id: {} ]",
        cancel_request.GetOrderId(), cancel_request.GetSessionId(),
        cancel_request.GetClientOrderId(),
        cancel_request.GetOrigClientOrderId());

    // We should either find or not find the cancel_request in both order maps
    if (found_order_id_map != found_clord_id_map) {
      spdlog::error(
          "MapListContainer::Remove inconsistent order map state: "
          "found_order_id_map {}, found_clord_id_map {}",
          found_order_id_map, found_clord_id_map);
    }

    return kFalsePair;
  }

  /**
   * Returns the first order in the list that is mapped to the first
   * key in the price_level_map.
   */
  auto Front() -> Order& { return *(price_level_map_.begin()->second.front()); }

  auto IsEmpty() -> bool { return size_ == 0; }
  auto Count() const -> std::size_t { return size_; }
  auto Clear() -> void {
    price_level_map_.clear();
    order_id_map_.clear();
    clord_id_map_.clear();
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
  /**
   * Generates a new intrusive_ptr from the pool and initializes it with the new
   * order single values.
   */
  static auto MakeOrder(const NewOrderSingle& new_order_single,
                        const OrderId& order_id) -> OrderPtr {
    auto ord = pool.MakeIntrusive();
    ord->SetOrderId(order_id)
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
        .SetClientOrderId(new_order_single.GetClientOrderId())
        .SetOrderStatus(OrderStatus::kPendingNew)
        .SetExecutedQuantity(0)
        .SetLastPrice(0)
        .SetLastQuantity(0)
        .SetExecutedValue(0)
        .Mark();

    return ord;
  }

  /**
   * Takes the modify request and resting order and updates the necessary data
   * structures to reflect the new client order state.
   */
  auto UpdateClientOrderId(const OrderCancelReplaceRequest& modify_request,
                           OrderPtr& order) -> void {
    // Erase the old key
    clord_id_map_.erase(
        {modify_request.GetSessionId(), modify_request.GetOrigClientOrderId()});

    // Add the new key
    const ClientOrderIdKey& new_key = {modify_request.GetSessionId(),
                                       modify_request.GetClientOrderId()};
    clord_id_map_.emplace(new_key, modify_request.GetSessionId());

    // Update the order
    order->SetClientOrderId(modify_request.GetClientOrderId())
        .SetOrigClientOrderId(modify_request.GetOrigClientOrderId());
  }

  /**
   * Adds the order into the order book w/o checking for valid state.
   */
  auto AddDirect(const OrderPtr& order) -> void {
    auto& list = price_level_map_[order->GetOrderPrice()];
    auto&& iter = list.insert(list.end(), std::move(order));
    order_id_map_.emplace(order->GetOrderId(), std::forward<Iterator>(iter));
  }

  /**
   * Removes the order from the order book w/o checking for valid state.
   */
  auto RemoveDirect(const OrderPtr& order) -> void {
    const auto& order_id_map_iter = order_id_map_.find(order->GetOrderId());
    auto& list = price_level_map_[order->GetOrderPrice()];

    list.erase(order_id_map_iter->second);
    order_id_map_.erase(order_id_map_iter);
    --size_;

    if (list.empty()) {
      price_level_map_.erase(order->GetOrderPrice());
    }
  }

  OrderIdMap order_id_map_{};
  PriceLevelMap price_level_map_{};
  ClientOrderIdMap clord_id_map_{};
  std::size_t size_{0};
};
}  // namespace orderbook::container
