#pragma once

#include <thread>

#include "orderbook/application_traits.h"
#include "orderbook/util/socket_providers.h"

namespace orderbook::gateway {

class OrderbookClient {
 private:
  using EventType = orderbook::data::EventType;
  using EventData = orderbook::data::EventData;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;
  using EventDispatcherPtr = std::shared_ptr<EventDispatcher>;
  using ClientSocket = orderbook::util::ClientSocketProvider;
  using ExecutionReport = orderbook::data::ExecutionReport;
  using OrderCancelReject = orderbook::data::OrderCancelReject;

  constexpr static std::size_t kBufferSize = 2048;

 public:
  OrderbookClient(EventDispatcherPtr dispatcher)
      : dispatcher_(std::move(dispatcher)) {}

  auto Connect(const std::string& addr) -> void {
    auto connected_event = [&](const zmq_event_t& event, const char* addr) {
      spdlog::info("client connected: addr {}, fd {}", addr, event.value);
    };

    socket_.Monitor(connected_event, ZMQ_EVENT_CONNECTED);
    socket_.Connect(addr);
  }

  auto ProcessMessages() -> void {
    auto message_handler = [&](zmq::message_t&& msg) {
      OnMessage(std::forward<zmq::message_t>(msg));
    };

    auto thread_handler = [&]() { socket_.ProcessMessages(message_handler); };

    recv_thread_ = std::thread(thread_handler);
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
      spdlog::warn("received orderbook::serialize::OrderPendingNew");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);

      spdlog::info(
          "got execution report: exec_id {}, order_id {}, ord_status {}, "
          "side {}, ord_qty {}, ord_prc {}, leaves_qty {}",
          exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
          exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
          exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
          exec_rpt.GetLeavesQuantity());

    } else if (event_type == orderbook::serialize::EventTypeCode::OrderNew) {
      spdlog::warn("received orderbook::serialize::OrderNew");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);

      spdlog::info(
          "got execution report: exec_id {}, order_id {}, ord_status {}, "
          "side {}, ord_qty {}, ord_prc {}, leaves_qty {}",
          exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
          exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
          exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
          exec_rpt.GetLeavesQuantity());

    } else if (event_type ==
               orderbook::serialize::EventTypeCode::OrderPartiallyFilled) {
      spdlog::warn("received orderbook::serialize::OrderPartiallyFilled");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);

      auto avg_px =
          (double)exec_rpt.GetExecutedValue() /
          (double)(exec_rpt.GetOrderQuantity() - exec_rpt.GetLeavesQuantity());

      spdlog::info(
          "got execution report: exec_id {}, order_id {}, ord_status {}, "
          "side {}, ord_qty {}, ord_prc {}, leaves_qty {}, avg_px {}",
          exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
          exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
          exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
          exec_rpt.GetLeavesQuantity(), avg_px);

    } else if (event_type == orderbook::serialize::EventTypeCode::OrderFilled) {
      spdlog::warn("received orderbook::serialize::OrderFilled");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);

      auto avg_px =
          (double)exec_rpt.GetExecutedValue() /
          (double)(exec_rpt.GetOrderQuantity() - exec_rpt.GetLeavesQuantity());

      spdlog::info(
          "got execution report: exec_id {}, order_id {}, ord_status {}, "
          "side {}, ord_qty {}, ord_prc {}, leaves_qty {}, avg_px {}",
          exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
          exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
          exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
          exec_rpt.GetLeavesQuantity(), avg_px);

    } else if (event_type ==
               orderbook::serialize::EventTypeCode::OrderCancelled) {
      spdlog::warn("received orderbook::serialize::OrderCancelled");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);

      spdlog::info(
          "got execution report: exec_id {}, order_id {}, ord_status {}",
          exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
          exec_rpt.GetOrderStatus());

    } else if (event_type ==
               orderbook::serialize::EventTypeCode::OrderRejected) {
      spdlog::warn("received orderbook::serialize::OrderRejected");
      const auto* exec_table = flatc_msg->body_as_ExecutionReport();
      auto exec_rpt = ExecutionReport(exec_table);

      spdlog::info("got execution report: {}", exec_rpt.GetExecutionId());

    } else if (event_type ==
               orderbook::serialize::EventTypeCode::OrderCancelRejected) {
      spdlog::warn("received orderbook::serialize::OrderCancelRejected");
      const auto* ord_cxl_rej_table = flatc_msg->body_as_OrderCancelReject();
      auto ord_cxl_rej = OrderCancelReject(ord_cxl_rej_table);

      spdlog::info("got order cancel reject: order_id {}",
                   ord_cxl_rej.GetOrderId());

    } else {
      spdlog::warn("received unknown orderbook::serialize::EventTypeCode");
    }
  }

  EventDispatcherPtr dispatcher_;
  ClientSocket socket_;
  std::thread recv_thread_;
  flatbuffers::FlatBufferBuilder builder{kBufferSize};
};

}  // namespace orderbook::gateway
