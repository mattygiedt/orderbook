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

          const auto& exec = std::get<ExecutionReport>(data);

          SerializeExecutionReport(builder_, EventType::kOrderPendingNew, exec);

          socket_.SendFlatBuffer(builder_.GetBufferPointer(),
                                 builder_.GetSize(), exec.GetRoutingId());
        });

    dispatcher_->appendListener(
        EventType::kOrderNew, [&](const EventData& data) {
          spdlog::info("EventType::kOrderNew");

          const auto& exec = std::get<ExecutionReport>(data);

          SerializeExecutionReport(builder_, EventType::kOrderNew, exec);

          socket_.SendFlatBuffer(builder_.GetBufferPointer(),
                                 builder_.GetSize(), exec.GetRoutingId());
        });

    dispatcher_->appendListener(
        EventType::kOrderPartiallyFilled, [&](const EventData& data) {
          spdlog::info("EventType::kOrderPartiallyFilled");

          const auto& exec = std::get<ExecutionReport>(data);

          SerializeExecutionReport(builder_, EventType::kOrderPartiallyFilled,
                                   exec);

          socket_.SendFlatBuffer(builder_.GetBufferPointer(),
                                 builder_.GetSize(), exec.GetRoutingId());
        });

    dispatcher_->appendListener(
        EventType::kOrderFilled, [&](const EventData& data) {
          spdlog::info("EventType::kOrderFilled");

          const auto& exec = std::get<ExecutionReport>(data);

          SerializeExecutionReport(builder_, EventType::kOrderFilled, exec);

          socket_.SendFlatBuffer(builder_.GetBufferPointer(),
                                 builder_.GetSize(), exec.GetRoutingId());
        });

    dispatcher_->appendListener(
        EventType::kOrderCancelled, [&](const EventData& data) {
          spdlog::info("EventType::kOrderCancelled");

          const auto& exec = std::get<ExecutionReport>(data);

          SerializeExecutionReport(builder_, EventType::kOrderCancelled, exec);

          socket_.SendFlatBuffer(builder_.GetBufferPointer(),
                                 builder_.GetSize(), exec.GetRoutingId());
        });

    dispatcher_->appendListener(
        EventType::kOrderRejected, [&](const EventData& data) {
          spdlog::info("EventType::kOrderRejected");

          const auto& exec = std::get<ExecutionReport>(data);

          SerializeExecutionReport(builder_, EventType::kOrderRejected, exec);

          socket_.SendFlatBuffer(builder_.GetBufferPointer(),
                                 builder_.GetSize(), exec.GetRoutingId());
        });

    dispatcher_->appendListener(
        EventType::kOrderCancelRejected, [&](const EventData& data) {
          spdlog::info("EventType::kOrderCancelRejected");

          const auto& ord_cxl_rej = std::get<OrderCancelReject>(data);

          SerializeOrderCancelReject(builder_, EventType::kOrderCancelRejected,
                                     ord_cxl_rej);

          socket_.SendFlatBuffer(builder_.GetBufferPointer(),
                                 builder_.GetSize(),
                                 ord_cxl_rej.GetRoutingId());
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

  OrderBook<typename orderbook::MapListOrderBookTraits> book(addr);
  book.GenerateOrderBooks();
  book.RegisterListeners();
  book.Run();

  return 0;
}
