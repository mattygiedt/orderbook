#pragma once

#include <thread>

#include "orderbook/application_traits.h"
#include "orderbook/util/socket_providers.h"

namespace orderbook::gateway {

class OrderbookClient {
 private:
  using TimeUtil = orderbook::util::TimeUtil;
  using EmptyType = orderbook::data::Empty;
  using EventType = orderbook::data::EventType;
  using EventData = orderbook::data::EventData;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;
  using EventDispatcherPtr = std::shared_ptr<EventDispatcher>;
  using ClientSocket = orderbook::util::ClientSocketProvider;
  using ExecutionReport = orderbook::data::ExecutionReport;
  using NewOrderSingle = orderbook::data::NewOrderSingle;
  using OrderCancelReject = orderbook::data::OrderCancelReject;
  using OrderCancelRequest = orderbook::data::OrderCancelRequest;
  using OrderCancelReplaceRequest = orderbook::data::OrderCancelReplaceRequest;

  constexpr static std::size_t kBufferSize = 2048;

 public:
  OrderbookClient(EventDispatcherPtr dispatcher)
      : dispatcher_(std::move(dispatcher)), data_{EmptyType()} {
    /**
     * Create listeners which forward create, modify, and delete
     * order requests to the orderbook.
     */
    dispatcher_->appendListener(
        EventType::kOrderPendingNew, [&](const EventData& data) {
          spdlog::info("OrderbookClient EventType::kOrderPendingNew");

          using namespace orderbook::serialize;

          auto& order = std::get<NewOrderSingle>(data);

          builder.Clear();
          builder.Finish(CreateMessage(
              builder,

              CreateHeader(builder, TimeUtil::EpochNanos(), ++seq_no_,
                           static_cast<orderbook::serialize::EventTypeCode>(
                               EventType::kOrderPendingNew)),

              Body::NewOrderSingle, order.SerializeTo(builder).Union()));

          socket_.SendFlatBuffer(builder.GetBufferPointer(), builder.GetSize());
        });

    dispatcher_->appendListener(
        EventType::kOrderPendingModify, [&](const EventData& data) {
          spdlog::info("OrderbookClient EventType::kOrderPendingModify");

          using namespace orderbook::serialize;

          auto& modify = std::get<OrderCancelReplaceRequest>(data);

          builder.Clear();
          builder.Finish(CreateMessage(
              builder,

              CreateHeader(builder, TimeUtil::EpochNanos(), ++seq_no_,
                           static_cast<orderbook::serialize::EventTypeCode>(
                               EventType::kOrderPendingModify)),

              Body::OrderCancelReplaceRequest,
              modify.SerializeTo(builder).Union()));

          socket_.SendFlatBuffer(builder.GetBufferPointer(), builder.GetSize());
        });

    dispatcher_->appendListener(
        EventType::kOrderPendingCancel, [&](const EventData& data) {
          spdlog::info("OrderbookClient EventType::kOrderPendingCancel");

          using namespace orderbook::serialize;

          auto& cancel = std::get<OrderCancelRequest>(data);

          builder.Clear();
          builder.Finish(CreateMessage(
              builder,

              CreateHeader(builder, TimeUtil::EpochNanos(), ++seq_no_,
                           static_cast<orderbook::serialize::EventTypeCode>(
                               EventType::kOrderPendingCancel)),

              Body::OrderCancelRequest, cancel.SerializeTo(builder).Union()));

          socket_.SendFlatBuffer(builder.GetBufferPointer(), builder.GetSize());
        });
  }

  auto Connect(const std::string& addr) -> void {
    auto connected_event = [&](const zmq_event_t& event, const char* addr) {
      spdlog::info("client connected: addr {}, fd {}", addr, event.value);
    };

    socket_.Monitor(connected_event, ZMQ_EVENT_CONNECTED);
    socket_.Connect(addr);
  }

  auto ProcessMessages() -> void {
    recv_thread_ = std::thread([&]() {
      socket_.ProcessMessages([&](zmq::message_t&& msg) {
        OnMessage(std::forward<zmq::message_t>(msg));
      });
    });
  }

  auto Close() -> void {
    socket_.Close();
    recv_thread_.join();
  }

 private:
  auto OnMessage(zmq::message_t&& msg) -> void {
    const auto* flatc_msg = orderbook::serialize::GetMessage(msg.data());
    auto event_type = flatc_msg->header()->event_type();

    if (event_type == orderbook::serialize::EventTypeCode::OrderPendingNew) {
      spdlog::info("received orderbook::serialize::OrderPendingNew");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);
      data_ = exec_rpt;
      dispatcher_->dispatch(EventType::kOrderPendingNew, data_);
    } else if (event_type == orderbook::serialize::EventTypeCode::OrderNew) {
      spdlog::info("received orderbook::serialize::OrderNew");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);
      data_ = exec_rpt;
      dispatcher_->dispatch(EventType::kOrderNew, data_);
    } else if (event_type ==
               orderbook::serialize::EventTypeCode::OrderPartiallyFilled) {
      spdlog::info("received orderbook::serialize::OrderPartiallyFilled");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);
      data_ = exec_rpt;
      dispatcher_->dispatch(EventType::kOrderPartiallyFilled, data_);
    } else if (event_type == orderbook::serialize::EventTypeCode::OrderFilled) {
      spdlog::info("received orderbook::serialize::OrderFilled");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);
      data_ = exec_rpt;
      dispatcher_->dispatch(EventType::kOrderFilled, data_);
    } else if (event_type ==
               orderbook::serialize::EventTypeCode::OrderModified) {
      spdlog::info("received orderbook::serialize::OrderModified");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);
      data_ = exec_rpt;
      dispatcher_->dispatch(EventType::kOrderModified, data_);
    } else if (event_type ==
               orderbook::serialize::EventTypeCode::OrderCancelled) {
      spdlog::info("received orderbook::serialize::OrderCancelled");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);
      data_ = exec_rpt;
      dispatcher_->dispatch(EventType::kOrderCancelled, data_);
    } else if (event_type ==
               orderbook::serialize::EventTypeCode::OrderRejected) {
      spdlog::warn("received orderbook::serialize::OrderRejected");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);
      data_ = exec_rpt;
      dispatcher_->dispatch(EventType::kOrderRejected, data_);
    } else if (event_type ==
               orderbook::serialize::EventTypeCode::OrderCancelRejected) {
      spdlog::warn("received orderbook::serialize::OrderCancelRejected");
      const auto* ord_cxl_rej_table = flatc_msg->body_as_OrderCancelReject();
      auto ord_cxl_rej = OrderCancelReject(ord_cxl_rej_table);
      data_ = ord_cxl_rej;
      dispatcher_->dispatch(EventType::kOrderCancelRejected, data_);
    } else {
      spdlog::warn("received unknown orderbook::serialize::EventTypeCode: {}",
                   event_type);
    }
  }

  EventDispatcherPtr dispatcher_;
  EventData data_;
  ClientSocket socket_;
  std::thread recv_thread_;
  flatbuffers::FlatBufferBuilder builder{kBufferSize};
  std::uint32_t seq_no_{0};
};

}  // namespace orderbook::gateway
