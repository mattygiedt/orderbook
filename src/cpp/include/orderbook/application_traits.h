#pragma once

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "eventpp/eventdispatcher.h"
#include "orderbook/book/book_concept.h"
#include "orderbook/book/limit_order_book.h"
#include "orderbook/container/container_concept.h"
#include "orderbook/container/intrusive_list_container.h"
#include "orderbook/container/intrusive_ptr_container.h"
#include "orderbook/container/map_list_container.h"
#include "orderbook/data/event_types.h"
#include "orderbook/data/limit_order.h"
#include "orderbook/data/object_pool.h"
#include "orderbook/serialize/orderbook_generated.h"
#include "orderbook/util/socket_providers.h"
#include "orderbook/util/time_util.h"
#include "spdlog/spdlog.h"

template <> struct fmt::formatter<orderbook::data::SideCode> : formatter<std::string_view> {
    using SideCode = orderbook::data::SideCode;

    template <typename FormatContext>
    auto format(SideCode c, FormatContext& ctx) {

        std::string_view name = "Unknown";
        switch (c) {
         case SideCode::kUnknown: break;
         case SideCode::kBuy: name = "Buy"; break;
         case SideCode::kSell: name = "Sell"; break;
         case SideCode::kSellShort: name = "SellShort"; break;
         case SideCode::kBuyCover: name = "BuyCover"; break;
    }
    return formatter<string_view>::format(name, ctx);
  }
};

template <> struct fmt::formatter<orderbook::data::OrderStatusCode> : formatter<std::string_view> {
    using OrderStatusCode = orderbook::data::OrderStatusCode;

    template <typename FormatContext>
    auto format(OrderStatusCode c, FormatContext& ctx) {

        std::string_view name = "Unknown";
        switch (c) {
         case OrderStatusCode::kUnknown: break;
         case OrderStatusCode::kPendingNew: name = "PendingNew"; break;
         case OrderStatusCode::kPendingModify: name = "PendingModify"; break;
         case OrderStatusCode::kPendingCancel: name = "PendingCancel"; break;
         case OrderStatusCode::kRejected: name = "Rejected"; break;
         case OrderStatusCode::kNew: name = "New"; break;
         case OrderStatusCode::kPartiallyFilled: name = "PartiallyFilled"; break;
         case OrderStatusCode::kFilled: name = "Filled"; break;
         case OrderStatusCode::kCancelled: name = "Cancelled"; break;
         case OrderStatusCode::kCompleted: name = "Completed"; break;
         case OrderStatusCode::kCancelRejected: name = "CancelRejected"; break;
    }
    return formatter<string_view>::format(name, ctx);
  }
};

template <> struct fmt::formatter<orderbook::serialize::EventTypeCode> : formatter<std::string_view> {
    using EventTypeCode = orderbook::serialize::EventTypeCode;

    template <typename FormatContext>
    auto format(EventTypeCode c, FormatContext& ctx) {

        std::string_view name = "Unknown";
        switch (c) {
         case EventTypeCode::Unknown: break;
         case EventTypeCode::OrderPendingNew: name = "OrderPendingNew"; break;
         case EventTypeCode::OrderPendingModify: name = "OrderPendingModify"; break;
         case EventTypeCode::OrderPendingCancel: name = "OrderPendingCancel"; break;
         case EventTypeCode::OrderRejected: name = "OrderRejected"; break;
         case EventTypeCode::OrderNew: name = "OrderNew"; break;
         case EventTypeCode::OrderPartiallyFilled: name = "OrderPartiallyFilled"; break;
         case EventTypeCode::OrderFilled: name = "OrderFilled"; break;
         case EventTypeCode::OrderCancelled: name = "OrderCancelled"; break;
         case EventTypeCode::OrderCompleted: name = "OrderCompleted"; break;
         case EventTypeCode::OrderCancelRejected: name = "OrderCancelRejected"; break;
         case EventTypeCode::OrderModified: name = "OrderModified"; break;
         case EventTypeCode::CancelOnDisconnect: name = "CancelOnDisconnect"; break;
    }
    return formatter<string_view>::format(name, ctx);
  }
};

namespace orderbook {

struct MapListOrderBookTraits {
  using PriceLevelKey = orderbook::data::Price;
  using OrderType = orderbook::data::LimitOrder;

  using EventType = orderbook::data::EventType;
  using EventData = orderbook::data::EventData;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;

  using BidContainerType =
      orderbook::container::MapListContainer<PriceLevelKey, std::greater<>>;
  using AskContainerType =
      orderbook::container::MapListContainer<PriceLevelKey, std::less<>>;

  using BookType =
      orderbook::book::LimitOrderBook<BidContainerType, AskContainerType,
                                      OrderType, EventDispatcher>;
};

template <std::size_t PoolSize = 16384>
struct IntrusivePtrOrderBookTraits {
  using PriceLevelKey = orderbook::data::Price;
  using OrderType = orderbook::data::IntrusiveLimitOrder<PoolSize>;
  using PoolType =
      orderbook::data::IntrusivePool<OrderType, OrderType::GetPoolSize()>;
  using EventType = orderbook::data::EventType;
  using EventData = orderbook::data::EventData;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;

  using BidContainerType =
      orderbook::container::IntrusivePtrContainer<PriceLevelKey, OrderType,
                                                  PoolType, std::greater<>>;
  using AskContainerType =
      orderbook::container::IntrusivePtrContainer<PriceLevelKey, OrderType,
                                                  PoolType, std::less<>>;

  using BookType =
      orderbook::book::LimitOrderBook<BidContainerType, AskContainerType,
                                      OrderType, EventDispatcher>;
};

template <std::size_t PoolSize = 16384>
struct IntrusiveListOrderBookTraits {
  using PriceLevelKey = orderbook::data::Price;
  using OrderType = orderbook::data::IntrusiveListLimitOrder<PoolSize>;
  using PoolType =
      orderbook::data::IntrusiveListPool<OrderType, OrderType::GetPoolSize()>;
  using EventType = orderbook::data::EventType;
  using EventData = orderbook::data::EventData;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;

  using BidContainerType =
      orderbook::container::IntrusiveListContainer<PriceLevelKey, OrderType,
                                                   PoolType, std::greater<>>;
  using AskContainerType =
      orderbook::container::IntrusiveListContainer<PriceLevelKey, OrderType,
                                                   PoolType, std::less<>>;

  using BookType =
      orderbook::book::LimitOrderBook<BidContainerType, AskContainerType,
                                      OrderType, EventDispatcher>;
};

}  // namespace orderbook
