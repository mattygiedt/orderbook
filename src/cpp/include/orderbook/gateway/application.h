#pragma once

#include "orderbook/application_traits.h"
#include "quickfix/Application.h"
#include "quickfix/Message.h"
#include "quickfix/Session.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/MessageCracker.h"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/OrderCancelReject.h"
#include "quickfix/fix42/OrderCancelReplaceRequest.h"
#include "quickfix/fix42/OrderCancelRequest.h"

namespace orderbook::gateway {

using namespace orderbook::data;

class GatewayApplication : public FIX::Application,
                           public FIX42::MessageCracker {
 private:
  using EmptyType = orderbook::data::Empty;
  using EventCallback = orderbook::data::EventCallback;
  using EventDispatcher = eventpp::EventDispatcher<EventType, EventCallback>;
  using EventDispatcherPtr = std::shared_ptr<EventDispatcher>;
  using FixSessionIdMap = std::map<FIX::SessionID, SessionId>;
  using ClientSessionIdMap = std::map<SessionId, FIX::SessionID>;

 public:
  GatewayApplication(EventDispatcherPtr dispatcher)
      : dispatcher_(std::move(dispatcher)), data_{EmptyType()} {
    dispatcher_->appendListener(
        EventType::kOrderNew, [&](const EventData& data) {
          auto& exec_rpt = std::get<ExecutionReport>(data);

          spdlog::info("GatewayApplication EventType::kOrderNew");
          spdlog::info(
              " execution report: exec_id {}, order_id {}, ord_status {}, "
              "side {}, ord_qty {}, ord_prc {}, leaves_qty {}, avg_px {}",
              exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
              exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
              exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
              exec_rpt.GetLeavesQuantity(), exec_rpt.GetAveragePrice());

          assert(exec_rpt.GetOrderStatus() == OrderStatus::kNew);
          assert(exec_rpt.HasOrigClientOrderId() == false);

          SendFixMessage(exec_rpt, FIX::ExecType(FIX::ExecType_NEW));
        });

    dispatcher_->appendListener(
        EventType::kOrderPartiallyFilled, [&](const EventData& data) {
          auto& exec_rpt = std::get<ExecutionReport>(data);

          spdlog::info("GatewayApplication EventType::kOrderPartiallyFilled");
          spdlog::info(
              " execution report: exec_id {}, order_id {}, ord_status {}, "
              "side {}, ord_qty {}, ord_prc {}, leaves_qty {}, avg_px {}",
              exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
              exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
              exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
              exec_rpt.GetLeavesQuantity(), exec_rpt.GetAveragePrice());

          assert(exec_rpt.GetOrderStatus() == OrderStatus::kPartiallyFilled);

          SendFixMessage(exec_rpt, FIX::ExecType(FIX::ExecType_PARTIAL_FILL));
        });

    dispatcher_->appendListener(
        EventType::kOrderFilled, [&](const EventData& data) {
          auto& exec_rpt = std::get<ExecutionReport>(data);

          spdlog::info("GatewayApplication EventType::kOrderFilled");
          spdlog::info(
              " execution report: exec_id {}, order_id {}, ord_status {}, "
              "side {}, ord_qty {}, ord_prc {}, leaves_qty {}, avg_px {}",
              exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
              exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
              exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
              exec_rpt.GetLeavesQuantity(), exec_rpt.GetAveragePrice());

          assert(exec_rpt.GetOrderStatus() == OrderStatus::kFilled);

          SendFixMessage(exec_rpt, FIX::ExecType(FIX::ExecType_FILL));
        });

    dispatcher_->appendListener(
        EventType::kOrderModified, [&](const EventData& data) {
          auto& exec_rpt = std::get<ExecutionReport>(data);

          spdlog::info("GatewayApplication EventType::kOrderModified");
          spdlog::info(
              " execution report: exec_id {}, order_id {}, ord_status {}, "
              "side {}, ord_qty {}, ord_prc {}, leaves_qty {}, avg_px {}",
              exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
              exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
              exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
              exec_rpt.GetLeavesQuantity(), exec_rpt.GetAveragePrice());

          SendFixMessage(exec_rpt, FIX::ExecType(FIX::ExecType_REPLACED));
        });

    dispatcher_->appendListener(
        EventType::kOrderCancelled, [&](const EventData& data) {
          auto& exec_rpt = std::get<ExecutionReport>(data);

          spdlog::info("GatewayApplication EventType::kOrderCancelled");
          spdlog::info(
              " execution report: exec_id {}, order_id {}, ord_status {}, "
              "side {}, ord_qty {}, ord_prc {}, leaves_qty {}, avg_px {}",
              exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
              exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
              exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
              exec_rpt.GetLeavesQuantity(), exec_rpt.GetAveragePrice());

          assert(exec_rpt.GetOrderStatus() == OrderStatus::kCancelled);

          SendFixMessage(exec_rpt, FIX::ExecType(FIX::ExecType_CANCELLED));
        });

    dispatcher_->appendListener(
        EventType::kOrderRejected, [&](const EventData& data) {
          auto& exec_rpt = std::get<ExecutionReport>(data);

          spdlog::info("GatewayApplication EventType::kOrderRejected");
          spdlog::info(
              " execution report: exec_id {}, order_id {}, ord_status {}, "
              "side {}, ord_qty {}, ord_prc {}, leaves_qty {}, avg_px {}",
              exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
              exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
              exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
              exec_rpt.GetLeavesQuantity(), exec_rpt.GetAveragePrice());

          assert(exec_rpt.GetOrderStatus() == OrderStatus::kRejected);

          SendFixMessage(exec_rpt, FIX::ExecType(FIX::ExecType_REJECTED));
        });

    dispatcher_->appendListener(
        EventType::kOrderCancelRejected, [&](const EventData& data) {
          auto& ord_cxl_rej = std::get<OrderCancelReject>(data);

          spdlog::info("GatewayApplication EventType::kOrderCancelRejected");
          spdlog::info(
              " order cancel reject: order_id {}, ord_status {}, "
              "clord_id {}, orig_clord_id {}",
              ord_cxl_rej.GetOrderId(), ord_cxl_rej.GetOrderStatus(),
              ord_cxl_rej.GetClientOrderId(),
              ord_cxl_rej.GetOrigClientOrderId());

          SendFixMessage(ord_cxl_rej);
        });
  }

  auto onCreate(const FIX::SessionID& session_id) -> void override {
    spdlog::info("session created: {}", session_id.toString());
  }

  auto onLogon(const FIX::SessionID& session_id) -> void override {
    fix_session_map_.emplace(session_id, ++next_session_id_);
    client_session_map_.emplace(next_session_id_, session_id);

    spdlog::info("session logon: session {} -> id {}", session_id.toString(),
                 fix_session_map_[session_id]);
  }

  auto onLogout(const FIX::SessionID& session_id) -> void override {
    spdlog::info("session logout: session {} -> id {}", session_id.toString(),
                 fix_session_map_[session_id]);

    orderbook::data::OrderCancelRequest cancel;
    cancel.SetSessionId(Convert(session_id));

    data_ = cancel;
    dispatcher_->dispatch(EventType::kCancelOnDisconnect, data_);

    client_session_map_.erase(fix_session_map_.at(session_id));
    fix_session_map_.erase(session_id);
  }

  auto toAdmin(FIX::Message& /*unused*/, const FIX::SessionID& /*unused*/)
      -> void override {}

  auto fromAdmin(const FIX::Message& /*unused*/,
                 const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::RejectLogon) -> void override {}

  auto toApp(FIX::Message& /*unused*/, const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::DoNotSend) -> void override {}

  auto fromApp(const FIX::Message& message, const FIX::SessionID& session_id)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::UnsupportedMessageType)
          -> void override {
    crack(message, session_id);
  }

 private:
  auto GetSymbol(const InstrumentId& /*unused*/) -> FIX::Symbol {
    return FIX::Symbol("AAPL");
  }

  auto Convert(const FIX::Side& side) const -> SideCode {
    switch (side) {
      case FIX::Side_BUY:
        return SideCode::kBuy;
      case FIX::Side_SELL:
        return SideCode::kSell;
      default:
        throw std::logic_error("Unsupported Side, use buy or sell");
    }
  }

  auto Convert(const SideCode& side) const -> FIX::Side {
    switch (side) {
      case SideCode::kBuy:
        return FIX::Side_BUY;
      case SideCode::kSell:
        return FIX::Side_SELL;
      default:
        throw std::logic_error("Unsupported Side, use buy or sell");
    }
  }

  auto Convert(const CxlRejResponseTo& status) const -> FIX::CxlRejResponseTo {
    switch (status) {
      case CxlRejResponseTo::kOrderCancelReplaceRequest:
        return FIX::CxlRejResponseTo_ORDER_CANCEL_REPLACE_REQUEST;
      case CxlRejResponseTo::kOrderCancelRequest:
        return FIX::CxlRejResponseTo_ORDER_CANCEL_REQUEST;
      default:
        throw std::logic_error("Unsupported cancel reject response to status");
    }
  }

  auto Convert(const OrderStatus& status) const -> FIX::OrdStatus {
    switch (status) {
      case OrderStatus::kNew:
        return FIX::OrdStatus_NEW;
      case OrderStatus::kPartiallyFilled:
        return FIX::OrdStatus_PARTIALLY_FILLED;
      case OrderStatus::kFilled:
        return FIX::OrdStatus_FILLED;
      case OrderStatus::kCancelled:
        return FIX::OrdStatus_CANCELED;
      case OrderStatus::kRejected:
      case OrderStatus::kCancelRejected:
        return FIX::OrdStatus_REJECTED;
      case OrderStatus::kCompleted:
        return FIX::OrdStatus_DONE_FOR_DAY;
      default:
        throw std::logic_error("Unsupported order status");
    }
  }

  auto Convert(const FIX::SessionID& session_id) -> SessionId {
    return fix_session_map_.at(session_id);
  }

  auto Convert(const FIX::Account& account) const -> AccountId {
    return std::stoi(account.getValue());
  }

  auto Convert(const FIX::OrderID& order_id) const -> OrderId {
    return std::stoi(order_id.getValue());
  }

  auto Convert(const FIX::SecurityID& securityId) const -> InstrumentId {
    return std::stoi(securityId.getValue());
  }

  auto onMessage(const FIX42::NewOrderSingle& message,
                 const FIX::SessionID& session_id) -> void override {
    spdlog::info("onMessage[{}] FIX42::NewOrderSingle: {}",
                 session_id.toString(), message.toString());

    FIX::SecurityID security_id;
    FIX::Side side;
    FIX::OrdType ord_type;
    FIX::OrderQty order_qty;
    FIX::Price price;
    FIX::ClOrdID clord_id;
    FIX::Account account_id;

    message.get(ord_type);

    if (ord_type != FIX::OrdType_LIMIT) {
      throw FIX::IncorrectTagValue(ord_type.getField());
    }

    message.get(security_id);
    message.get(side);
    message.get(order_qty);
    message.get(price);
    message.get(clord_id);
    message.get(account_id);

    const auto& prc = orderbook::data::ToPrice(price.getValue());

    orderbook::data::NewOrderSingle order;
    order.SetOrderPrice(prc)
        .SetOrderQuantity(order_qty.getValue())
        .SetSide(Convert(side))
        .SetInstrumentId(Convert(security_id))
        .SetSessionId(Convert(session_id))
        .SetAccountId(Convert(account_id))
        .SetClientOrderId(clord_id.getValue())
        .SetOrderType(OrderType::kLimit)
        .SetTimeInForce(TimeInForce::kDay)
        .SetOrderStatus(OrderStatus::kPendingNew);

    data_ = order;
    dispatcher_->dispatch(EventType::kOrderPendingNew, data_);
  }

  auto onMessage(const FIX42::OrderCancelReplaceRequest& message,
                 const FIX::SessionID& session_id) -> void override {
    spdlog::info("onMessage[{}] FIX42::OrderCancelReplaceRequest: {}",
                 session_id.toString(), message.toString());
    FIX::OrigClOrdID orig_clord_id;
    FIX::ClOrdID clord_id;
    FIX::Side side;
    FIX::Price price;
    FIX::SecurityID security_id;
    FIX::OrderID order_id;
    FIX::Account account_id;
    FIX::OrderQty order_qty;

    message.get(orig_clord_id);
    message.get(clord_id);
    message.get(side);
    message.get(security_id);
    message.get(order_id);
    message.get(account_id);
    message.get(order_qty);
    message.get(price);

    const auto& prc = orderbook::data::ToPrice(price.getValue());

    orderbook::data::OrderCancelReplaceRequest modify;
    modify.SetOrderPrice(prc)
        .SetAccountId(Convert(account_id))
        .SetOrderQuantity(order_qty.getValue())
        .SetSide(Convert(side))
        .SetInstrumentId(Convert(security_id))
        .SetSessionId(Convert(session_id))
        .SetOrderId(Convert(order_id))
        .SetClientOrderId(clord_id.getValue())
        .SetOrigClientOrderId(orig_clord_id.getValue());

    data_ = modify;
    dispatcher_->dispatch(EventType::kOrderPendingModify, data_);
  }

  auto onMessage(const FIX42::OrderCancelRequest& message,
                 const FIX::SessionID& session_id) -> void override {
    spdlog::info("onMessage[{}] FIX42::OrderCancelRequest: {}",
                 session_id.toString(), message.toString());
    FIX::OrigClOrdID orig_clord_id;
    FIX::ClOrdID clord_id;
    FIX::Side side;
    FIX::SecurityID security_id;
    FIX::OrderID order_id;
    FIX::Account account_id;
    FIX::OrderQty order_qty;

    message.get(orig_clord_id);
    message.get(clord_id);
    message.get(side);
    message.get(security_id);
    message.get(order_id);
    message.get(account_id);
    message.get(order_qty);

    orderbook::data::OrderCancelRequest cancel;
    cancel.SetAccountId(Convert(account_id))
        .SetOrderQuantity(order_qty.getValue())
        .SetSide(Convert(side))
        .SetInstrumentId(Convert(security_id))
        .SetSessionId(Convert(session_id))
        .SetOrderId(Convert(order_id))
        .SetClientOrderId(clord_id.getValue())
        .SetOrigClientOrderId(orig_clord_id.getValue());

    data_ = cancel;
    dispatcher_->dispatch(EventType::kOrderPendingCancel, data_);
  }

  auto SendFixMessage(const ExecutionReport& exec_rpt,
                      const FIX::ExecType& exec_type) -> void {
    const auto& px = orderbook::data::ToDouble(exec_rpt.GetOrderPrice());
    const auto& avg_px = orderbook::data::ToDouble(exec_rpt.GetAveragePrice());
    const auto& last_px = orderbook::data::ToDouble(exec_rpt.GetLastPrice());

    FIX42::ExecutionReport executionReport = FIX42::ExecutionReport(
        FIX::OrderID(std::to_string(exec_rpt.GetOrderId())),
        FIX::ExecID(std::to_string(exec_rpt.GetExecutionId())),
        FIX::ExecTransType(FIX::ExecTransType_NEW), exec_type,
        Convert(exec_rpt.GetOrderStatus()),
        GetSymbol(exec_rpt.GetInstrumentId()), Convert(exec_rpt.GetSide()),
        FIX::LeavesQty(exec_rpt.GetLeavesQuantity()),
        FIX::CumQty(exec_rpt.GetExecutedQuantity()), FIX::AvgPx(avg_px));

    executionReport.set(FIX::Price(px));
    executionReport.set(FIX::LastPx(last_px));
    executionReport.set(FIX::ClOrdID(exec_rpt.GetClientOrderId()));
    executionReport.set(FIX::OrderQty(exec_rpt.GetOrderQuantity()));
    executionReport.set(FIX::LastShares(exec_rpt.GetLastQuantity()));
    executionReport.set(FIX::Account(std::to_string(exec_rpt.GetAccountId())));
    executionReport.set(
        FIX::SecurityID(std::to_string(exec_rpt.GetInstrumentId())));
    executionReport.set(FIX::IDSource(FIX::IDSource_EXCHANGE_SYMBOL));

    if (exec_rpt.HasOrigClientOrderId()) {
      executionReport.set(FIX::OrigClOrdID(exec_rpt.GetOrigClientOrderId()));
    }

    const FIX::SessionID& session_id =
        client_session_map_.at(exec_rpt.GetSessionId());
    FIX::Session::sendToTarget(executionReport, session_id);
  }

  auto SendFixMessage(const OrderCancelReject& ord_cxl_rej) -> void {
    FIX42::OrderCancelReject orderCancelReject = FIX42::OrderCancelReject(
        FIX::OrderID(std::to_string(ord_cxl_rej.GetOrderId())),
        FIX::ClOrdID(ord_cxl_rej.GetClientOrderId()),
        FIX::OrigClOrdID(ord_cxl_rej.GetOrigClientOrderId()),
        Convert(ord_cxl_rej.GetOrderStatus()),
        Convert(ord_cxl_rej.GetCxlRejResponseTo()));

    const FIX::SessionID& session_id =
        client_session_map_.at(ord_cxl_rej.GetSessionId());
    FIX::Session::sendToTarget(orderCancelReject, session_id);
  }

  FixSessionIdMap fix_session_map_{};
  ClientSessionIdMap client_session_map_{};
  EventDispatcherPtr dispatcher_;
  EventData data_;
  SessionId next_session_id_{0};
};

}  // namespace orderbook::gateway
