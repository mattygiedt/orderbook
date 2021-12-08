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
#include "orderbook/container/map_list_container.h"
#include "orderbook/data/event_types.h"
#include "orderbook/data/limit_order.h"
#include "orderbook/data/object_pool.h"
#include "orderbook/serialize/orderbook_generated.h"
#include "orderbook/util/socket_providers.h"
#include "orderbook/util/time_util.h"
#include "spdlog/spdlog.h"

namespace orderbook {

template <std::size_t PoolSize = 16384>
struct MapListOrderBookTraits {
  using PriceLevelKey = orderbook::data::Price;
  using OrderType = orderbook::data::IntrusiveLimitOrder<PoolSize>;
  using PoolType =
      orderbook::data::IntrusivePool<OrderType, OrderType::GetPoolSize()>;
  using EventType = orderbook::data::EventType;
  using EventData = orderbook::data::EventData;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;

  using BidContainerType =
      orderbook::container::MapListContainer<PriceLevelKey, OrderType, PoolType,
                                             std::greater<>>;
  using AskContainerType =
      orderbook::container::MapListContainer<PriceLevelKey, OrderType, PoolType,
                                             std::less<>>;

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
