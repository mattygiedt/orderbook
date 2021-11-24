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
#include "orderbook/container/map_list_container.h"
#include "orderbook/data/event_types.h"
#include "orderbook/data/limit_order.h"
#include "orderbook/serialize/orderbook_generated.h"
#include "orderbook/util/socket_providers.h"
#include "orderbook/util/time_util.h"
#include "spdlog/spdlog.h"

namespace orderbook {
template <typename OrderType>
struct ContainerTraits {
  using MapListKey = orderbook::data::Price;
  using MapListBidContainer =
      orderbook::container::MapListContainer<MapListKey, OrderType,
                                             std::greater<MapListKey>>;
  using MapListAskContainer =
      orderbook::container::MapListContainer<MapListKey, OrderType,
                                             std::less<MapListKey>>;
};

template <typename BidContainerType, typename AskContainerType,
          typename OrderType, typename EventDispatcher>
struct BookTraits {
  using OrderBook =
      orderbook::book::LimitOrderBook<BidContainerType, AskContainerType,
                                      OrderType, EventDispatcher>;
};

struct MapListOrderBookTraits {
  using OrderType = orderbook::data::LimitOrder;
  using EventType = orderbook::data::EventType;
  using EventData = orderbook::data::EventData;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;

  using BidContainerType = ContainerTraits<OrderType>::MapListBidContainer;
  using AskContainerType = ContainerTraits<OrderType>::MapListAskContainer;
  using BookType = BookTraits<BidContainerType, AskContainerType, OrderType,
                              EventDispatcher>::OrderBook;
};

}  // namespace orderbook
