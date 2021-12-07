#pragma once

#include "orderbook/application_traits.h"
#include "quickfix/Application.h"
#include "quickfix/Message.h"
#include "quickfix/Session.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/MessageCracker.h"
#include "quickfix/fix42/NewOrderSingle.h"

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

          spdlog::info("OrderbookClient EventType::kOrderNew");
          spdlog::info(
              "got execution report: exec_id {}, order_id {}, ord_status {}, "
              "side {}, ord_qty {}, ord_prc {}, leaves_qty {}",
              exec_rpt.GetExecutionId(), exec_rpt.GetOrderId(),
              exec_rpt.GetOrderStatus(), exec_rpt.GetSide(),
              exec_rpt.GetOrderQuantity(), exec_rpt.GetOrderPrice(),
              exec_rpt.GetLeavesQuantity());

          FIX42::ExecutionReport executionReport = FIX42::ExecutionReport(
              FIX::OrderID(std::to_string(exec_rpt.GetOrderId())),
              FIX::ExecID(std::to_string(exec_rpt.GetExecutionId())),
              FIX::ExecTransType(FIX::ExecTransType_NEW),
              FIX::ExecType(FIX::ExecType_NEW),
              FIX::OrdStatus(FIX::OrdStatus_NEW), FIX::Symbol("INVALID_SYMBOL"),
              Convert(exec_rpt.GetSide()),
              FIX::LeavesQty(exec_rpt.GetLeavesQuantity()),
              FIX::CumQty(exec_rpt.GetOrderQuantity()),
              FIX::AvgPx(
                  orderbook::data::ToDouble(exec_rpt.GetAveragePrice())));

          executionReport.set(FIX::ClOrdID(exec_rpt.GetClientOrderId()));
          executionReport.set(FIX::OrderQty(exec_rpt.GetOrderQuantity()));
          executionReport.set(FIX::LastShares(exec_rpt.GetLastQuantity()));
          executionReport.set(
              FIX::Account(std::to_string(exec_rpt.GetAccountId())));
          executionReport.set(
              FIX::LastPx(orderbook::data::ToDouble(exec_rpt.GetLastPrice())));
          executionReport.set(
              FIX::SecurityID(std::to_string(exec_rpt.GetInstrumentId())));
          executionReport.set(FIX::IDSource(FIX::IDSource_EXCHANGE_SYMBOL));

          const FIX::SessionID& session_id =
              client_session_map_.at(exec_rpt.GetSessionId());
          FIX::Session::sendToTarget(executionReport, session_id);
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

    client_session_map_.erase(fix_session_map_.at(session_id));
    fix_session_map_.erase(session_id);

    // TODO: cancel on disconnect
  }

  auto toAdmin(FIX::Message& message, const FIX::SessionID& /*unused*/)
      -> void override {
    spdlog::info("toAdmin: {}", message.toString());
  }

  auto fromAdmin(const FIX::Message& message, const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::RejectLogon) -> void override {
    spdlog::info("fromAdmin: {}", message.toString());
  }

  auto toApp(FIX::Message& message, const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::DoNotSend) -> void override {
    spdlog::info("toApp: {}", message.toString());
  }

  auto fromApp(const FIX::Message& message, const FIX::SessionID& session_id)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::UnsupportedMessageType)
          -> void override {
    spdlog::info("fromApp: {}", message.toString());
    crack(message, session_id);
  }

 private:
  auto Convert(const FIX::Side& side) -> SideCode {
    switch (side) {
      case FIX::Side_BUY:
        return SideCode::kBuy;
      case FIX::Side_SELL:
        return SideCode::kSell;
      default:
        throw std::logic_error("Unsupported Side, use buy or sell");
    }
  }

  auto Convert(const SideCode& side) -> FIX::Side {
    switch (side) {
      case SideCode::kBuy:
        return FIX::Side_BUY;
      case SideCode::kSell:
        return FIX::Side_SELL;
      default:
        throw std::logic_error("Unsupported Side, use buy or sell");
    }
  }

  auto Convert(const FIX::SessionID& session_id) -> SessionId {
    return fix_session_map_.at(session_id);
  }

  auto Convert(const FIX::Account& account) -> AccountId {
    return std::stoi(account.getValue());
  }

  auto Convert(const FIX::SecurityID& securityId) -> InstrumentId {
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
    FIX::Account account;

    message.get(ord_type);

    if (ord_type != FIX::OrdType_LIMIT) {
      throw FIX::IncorrectTagValue(ord_type.getField());
    }

    message.get(security_id);
    message.get(side);
    message.get(order_qty);
    message.get(price);
    message.get(clord_id);
    message.get(account);

    const auto& prc = orderbook::data::ToPrice(price.getValue());

    orderbook::data::NewOrderSingle order;
    order.SetOrderPrice(prc)
        .SetOrderQuantity(order_qty.getValue())
        .SetSide(Convert(side))
        .SetInstrumentId(Convert(security_id))
        .SetSessionId(Convert(session_id))
        .SetAccountId(Convert(account))
        .SetClientOrderId(clord_id.getValue())
        .SetOrderType(OrderType::kLimit)
        .SetTimeInForce(TimeInForce::kDay)
        .SetOrderStatus(OrderStatus::kPendingNew);

    data_ = order;
    dispatcher_->dispatch(EventType::kOrderPendingNew, data_);
  }

  FixSessionIdMap fix_session_map_{};
  ClientSessionIdMap client_session_map_{};
  EventDispatcherPtr dispatcher_;
  EventData data_;
  SessionId next_session_id_{0};
};

}  // namespace orderbook::gateway
