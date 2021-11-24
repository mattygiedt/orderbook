#include <iostream>
#include <map>
#include <string>

#include "orderbook/application_traits.h"

using namespace orderbook::data;
using namespace orderbook::util;

auto RandomStringView(const std::size_t length) -> std::string_view {
  auto rand_char = []() -> char {
    const char charset[] =  // NOLINT
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);
    return charset[rand() % max_index];
  };

  std::string rand_str(length, 0);
  std::generate_n(rand_str.begin(), length, rand_char);
  std::string_view str_view = rand_str;
  return str_view;
}

auto main(int argc, char** argv) -> int {
  if (argc < 2) {
    std::cout << "usage: " << argv[0] << " ADDR." << std::endl;
    return 1;
  }

  std::string addr(argv[1]);

  auto connected_event = [](const zmq_event_t& event, const char* addr) {
    spdlog::info("client connected: addr {}, fd {}", addr, event.value);
  };

  auto message_handler = [](zmq::message_t&& msg) {
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
  };

  constexpr static std::size_t kBufferSize = 2048;
  flatbuffers::FlatBufferBuilder builder{kBufferSize};

  using ClientSocket = orderbook::util::ClientSocketProvider;
  ClientSocket socket;

  auto send_new_order = [&](const Price prc, const Quantity qty,
                            const Side side, const ClientOrderId cl_ord_id) {
    NewOrderSingle order;
    order.SetOrderPrice(prc)
        .SetOrderQuantity(qty)
        .SetSide(side)
        .SetInstrumentId(1)
        .SetClientOrderId(cl_ord_id)
        .SetOrderType(OrderType::kLimit)
        .SetTimeInForce(TimeInForce::kDay)
        .SetOrderStatus(OrderStatus::kPendingNew);

    using namespace orderbook::serialize;

    builder.Clear();
    builder.Finish(CreateMessage(
        builder,

        CreateHeader(builder, TimeUtil::EpochNanos(), 1,
                     static_cast<orderbook::serialize::EventTypeCode>(
                         EventType::kOrderPendingNew)),

        Body::NewOrderSingle, order.SerializeTo(builder).Union()));

    socket.SendFlatBuffer(builder.GetBufferPointer(), builder.GetSize());
  };

  /*
    auto send_modify_order = [&](const Price prc, const Quantity qty,
                                 const Side side,
                                 const OrigClientOrderId orig_cl_ord_id,
                                 const ClientOrderId cl_ord_id,
                                 const OrderId id) {
      OrderCancelReplaceRequest modify;
      modify.SetOrderId(id)
          .SetOrderPrice(prc)
          .SetOrderQuantity(qty)
          .SetSide(side)
          .SetInstrumentId(1)
          .SetOrigClientOrderId(orig_cl_ord_id)
          .SetClientOrderId(cl_ord_id)
          .SetOrderType(OrderType::kLimit)
          .SetTimeInForce(TimeInForce::kDay)
          .SetOrderStatus(OrderStatus::kPendingModify);

      using namespace orderbook::serialize;

      builder.Clear();
      builder.Finish(CreateMessage(
          builder,

          CreateHeader(builder, TimeUtil::EpochNanos(), 1,
                       static_cast<orderbook::serialize::EventTypeCode>(
                           EventType::kOrderPendingModify)),

          Body::OrderCancelReplaceRequest,
    modify.SerializeTo(builder).Union()));

      socket.SendFlatBuffer(builder.GetBufferPointer(), builder.GetSize());
    };
  */

  auto send_cancel_order = [&](const Price prc, const Quantity qty,
                               const Side side, const ClientOrderId cl_ord_id,
                               const OrderId id) {
    OrderCancelRequest cancel;
    cancel.SetOrderId(id)
        .SetOrderPrice(prc)
        .SetOrderQuantity(qty)
        .SetSide(side)
        .SetInstrumentId(1)
        .SetOrigClientOrderId(cl_ord_id)
        .SetClientOrderId(RandomStringView(8))  // NOLINT
        .SetOrderType(OrderType::kLimit)
        .SetTimeInForce(TimeInForce::kDay)
        .SetOrderStatus(OrderStatus::kPendingCancel);

    using namespace orderbook::serialize;

    builder.Clear();
    builder.Finish(CreateMessage(
        builder,

        CreateHeader(builder, TimeUtil::EpochNanos(), 1,
                     static_cast<orderbook::serialize::EventTypeCode>(
                         EventType::kOrderPendingCancel)),

        Body::OrderCancelRequest, cancel.SerializeTo(builder).Union()));

    socket.SendFlatBuffer(builder.GetBufferPointer(), builder.GetSize());
  };

  socket.Monitor(connected_event, ZMQ_EVENT_CONNECTED);
  socket.Connect(addr);

  auto clord_id = RandomStringView(8);  // NOLINT

  // send_new_order(20, 15, Side::kBuy, clord_id);        // NOLINT
  // send_cancel_order(20, 15, Side::kBuy, clord_id, 1);  // NOLINT

  // send_modify_order(20, 15, Side::kBuy, 1);  // NOLINT
  // send_cancel_order(20, 15, Side::kBuy, 1);  // NOLINT

  send_new_order(20, 15, Side::kBuy, clord_id);              // NOLINT
  send_new_order(20, 15, Side::kSell, RandomStringView(8));  // NOLINT
  send_cancel_order(20, 15, Side::kBuy, clord_id, 1);        // NOLINT

  socket.ProcessMessages(message_handler);

  return 0;
}
