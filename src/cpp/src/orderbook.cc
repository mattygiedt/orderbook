#include <iostream>
#include <map>
#include <string>

#include "orderbook/application_traits.h"

using namespace orderbook::data;

// clang-format off
template <typename OrderBookTraits>

  requires
    ( orderbook::book::BookConcept<typename OrderBookTraits::BookType> &&
      orderbook::container::ContainerConcept<typename OrderBookTraits::BidContainerType,
                                             typename OrderBookTraits::OrderType> &&
      orderbook::container::ContainerConcept<typename OrderBookTraits::AskContainerType,
                                             typename OrderBookTraits::OrderType> )

class OrderBook {  // clang-format on
 private:
  using RoutingId = std::uint32_t;
  using SequenceNumber = std::uint32_t;
  using TimeUtil = orderbook::util::TimeUtil;
  using BookType = typename OrderBookTraits::BookType;
  using EventType = typename OrderBookTraits::EventType;
  using EventData = typename OrderBookTraits::EventData;
  using Order = typename OrderBookTraits::OrderType;
  using BookMap = std::unordered_map<InstrumentId, BookType>;
  using ServerSocket = orderbook::util::ServerSocketProvider;
  using EventDispatcher = typename OrderBookTraits::EventDispatcher;
  using EventDispatcherPtr = std::shared_ptr<EventDispatcher>;

 public:
  constexpr static std::size_t kBufferSize = 2048;
  constexpr static std::size_t kInstrumentCount = 2048;

  OrderBook(std::string addr)
      : dispatcher_(std::make_shared<EventDispatcher>()),
        addr_(std::move(addr)) {}

  auto GenerateOrderBooks() -> void {
    // Normally this would be driven by some rational symbology process.
    std::size_t instrument_id = 1;
    for (; instrument_id <= kInstrumentCount; ++instrument_id) {
      book_map_.emplace(std::make_pair(instrument_id, BookType(dispatcher_)));
    }
  }

  auto RegisterListeners() -> void {
    dispatcher_->appendListener(
        EventType::kOrderPendingNew, [&](const EventData& data) {
          spdlog::info("EventType::kOrderPendingNew");

          HandleExecutionReport(std::get<ExecutionReport>(data),
                                EventType::kOrderPendingNew);
        });

    dispatcher_->appendListener(
        EventType::kOrderNew, [&](const EventData& data) {
          spdlog::info("EventType::kOrderNew");

          HandleExecutionReport(std::get<ExecutionReport>(data),
                                EventType::kOrderNew);
        });

    dispatcher_->appendListener(
        EventType::kOrderPartiallyFilled, [&](const EventData& data) {
          spdlog::info("EventType::kOrderPartiallyFilled");

          HandleExecutionReport(std::get<ExecutionReport>(data),
                                EventType::kOrderPartiallyFilled);
        });

    dispatcher_->appendListener(
        EventType::kOrderFilled, [&](const EventData& data) {
          spdlog::info("EventType::kOrderFilled");

          HandleExecutionReport(std::get<ExecutionReport>(data),
                                EventType::kOrderFilled);
        });

    dispatcher_->appendListener(
        EventType::kOrderCancelled, [&](const EventData& data) {
          spdlog::info("EventType::kOrderCancelled");

          HandleExecutionReport(std::get<ExecutionReport>(data),
                                EventType::kOrderCancelled);
        });

    dispatcher_->appendListener(
        EventType::kOrderRejected, [&](const EventData& data) {
          spdlog::info("EventType::kOrderRejected");

          HandleExecutionReport(std::get<ExecutionReport>(data),
                                EventType::kOrderRejected);
        });

    dispatcher_->appendListener(
        EventType::kOrderModified, [&](const EventData& data) {
          spdlog::info("EventType::kOrderModified");

          HandleExecutionReport(std::get<ExecutionReport>(data),
                                EventType::kOrderModified);
        });

    dispatcher_->appendListener(
        EventType::kOrderCancelRejected, [&](const EventData& data) {
          spdlog::info("EventType::kOrderCancelRejected");

          HandleOrderCancelReject(std::get<OrderCancelReject>(data),
                                  EventType::kOrderCancelRejected);
        });
  }

  auto Run() -> void {
    auto socket_event = [](const zmq_event_t& event, const char* addr) {
      spdlog::info("event type {}, addr {}, fd {}", event.event, addr,
                   event.value);
    };

    spdlog::info("socket_.bind({})", addr_);
    socket_.Monitor(socket_event);
    socket_.Bind(addr_);
    socket_.ProcessMessages([&](zmq::message_t&& msg) {
      const auto* flatc_msg = orderbook::serialize::GetMessage(msg.data());
      auto event_type = flatc_msg->header()->event_type();

      if (event_type == orderbook::serialize::EventTypeCode::OrderPendingNew) {
        const auto* table = flatc_msg->body_as_NewOrderSingle();
        auto order = NewOrderSingle(table);
        order.SetRoutingId(msg.routing_id());
        book_map_.at(order.GetInstrumentId()).Add(order);
      } else if (event_type ==
                 orderbook::serialize::EventTypeCode::OrderPendingModify) {
        const auto* table = flatc_msg->body_as_OrderCancelReplaceRequest();
        auto modify = OrderCancelReplaceRequest(table);
        modify.SetRoutingId(msg.routing_id());
        book_map_.at(modify.GetInstrumentId()).Modify(modify);
      } else if (event_type ==
                 orderbook::serialize::EventTypeCode::OrderPendingCancel) {
        const auto* table = flatc_msg->body_as_OrderCancelRequest();
        auto cancel = OrderCancelRequest(table);
        cancel.SetRoutingId(msg.routing_id());
        book_map_.at(cancel.GetInstrumentId()).Cancel(cancel);
      } else {
        spdlog::warn("received unknown orderbook::serialize::EventTypeCode");
        // TODO: Send Reject
        return;
      }
    });
  }

 private:
  auto GetSerializedEventType(const EventType& event_type) const
      -> orderbook::serialize::EventTypeCode {
    return static_cast<orderbook::serialize::EventTypeCode>(event_type);
  }

  auto SerializeExecutionReport(flatbuffers::FlatBufferBuilder& builder,
                                const EventType& event_type,
                                const ExecutionReport& execution_report)
      -> void {
    using namespace orderbook::serialize;

    builder.Clear();
    builder.Finish(CreateMessage(
        builder_,
        CreateHeader(builder, TimeUtil::EpochNanos(), ++seq_no_,
                     GetSerializedEventType(event_type)),
        Body::ExecutionReport, execution_report.SerializeTo(builder).Union()));
  }

  auto SerializeOrderCancelReject(flatbuffers::FlatBufferBuilder& builder,
                                  const EventType& event_type,
                                  const OrderCancelReject& order_cancel_reject)
      -> void {
    using namespace orderbook::serialize;

    builder.Clear();
    builder.Finish(
        CreateMessage(builder_,
                      CreateHeader(builder, TimeUtil::EpochNanos(), ++seq_no_,
                                   GetSerializedEventType(event_type)),
                      Body::OrderCancelReject,
                      order_cancel_reject.SerializeTo(builder).Union()));
  }

  auto HandleExecutionReport(const ExecutionReport& execution_report,
                             const EventType& event_type) -> void {
    SerializeExecutionReport(builder_, event_type, execution_report);

    socket_.SendFlatBuffer(builder_.GetBufferPointer(), builder_.GetSize(),
                           execution_report.GetRoutingId());
  }

  auto HandleOrderCancelReject(const OrderCancelReject& order_cancel_reject,
                               const EventType& event_type) -> void {
    SerializeOrderCancelReject(builder_, event_type, order_cancel_reject);

    socket_.SendFlatBuffer(builder_.GetBufferPointer(), builder_.GetSize(),
                           order_cancel_reject.GetRoutingId());
  }

  EventDispatcherPtr dispatcher_;
  std::string addr_;
  ServerSocket socket_;
  BookMap book_map_;

  SequenceNumber seq_no_{0};
  flatbuffers::FlatBufferBuilder builder_{kBufferSize};
};

auto main(int argc, char** argv) -> int {
  if (argc < 2) {
    std::cout << "usage: " << argv[0] << " ADDR." << std::endl;
    return 1;
  }

  std::string addr(argv[1]);

  // OrderBook<typename orderbook::MapListOrderBookTraits<>> book(addr);
  OrderBook<typename orderbook::IntrusiveListOrderBookTraits<>> book(addr);
  book.GenerateOrderBooks();
  book.RegisterListeners();
  book.Run();

  return 0;
}
