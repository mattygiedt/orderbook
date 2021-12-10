#include <iostream>
#include <map>
#include <string>

#include "quickfix/Application.h"
#include "quickfix/FileLog.h"
#include "quickfix/FileStore.h"
#include "quickfix/SessionSettings.h"
#include "quickfix/SocketInitiator.h"
#include "quickfix/config.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/MessageCracker.h"
#include "quickfix/fix42/NewOrderSingle.h"
#include "quickfix/fix42/OrderCancelReject.h"
#include "quickfix/fix42/OrderCancelReplaceRequest.h"
#include "quickfix/fix42/OrderCancelRequest.h"
#include "spdlog/spdlog.h"

static constexpr auto kClientOrderIdSize = 8;

static auto MakeClientOrderId(const std::size_t& length) -> std::string {
  static std::size_t clord_id{0};
  const auto& id = std::to_string(++clord_id);
  auto clord_id_str =
      std::string(length - std::min(length, id.length()), '0') + id;
  return clord_id_str;
}

class FixClient : public FIX::Application, public FIX42::MessageCracker {
 private:
  inline static constexpr char kHandleInstr =
      FIX::HandlInst_AUTOMATED_EXECUTION_ORDER_PRIVATE_NO_BROKER_INTERVENTION;

 public:
  FixClient(std::string config)
      : config_(std::move(config)), initiator_{nullptr} {}

  auto Initialize() -> void {
    FIX::SessionSettings settings(config_);
    FIX::FileStoreFactory store_factory(settings);
    FIX::ScreenLogFactory log_factory(settings);

    initiator_ = std::make_unique<FIX::SocketInitiator>(*this, store_factory,
                                                        settings, log_factory);
  }

  auto Start() -> void { initiator_->block(); }

  auto Stop() -> void { initiator_->stop(); }

  auto onCreate(const FIX::SessionID& session_id) -> void override {
    spdlog::info("session created: {}", session_id.toString());
    session_id_ = session_id;
  }

  auto onLogon(const FIX::SessionID& session_id) -> void override {
    spdlog::info("session logon: {}", session_id.toString());
    SendNewOrderRequest(123.456, 42, "AAPL", 1999, 1, FIX::Side_BUY);  // NOLINT
    SendCancelOrderRequest(42, "AAPL", 1999, 1, FIX::Side_BUY, 1,      // NOLINT
                           "00000001");                                // NOLINT
    SendCancelOrderRequest(42, "AAPL", 1999, 1, FIX::Side_BUY, 1,      // NOLINT
                           "00000001");                                // NOLINT
                                                                       //
    //    SendNewOrderRequest(123.456, 100, "AAPL", 1999, 1,  // NOLINT
    //                        FIX::Side_BUY);                 // NOLINT
    //    SendNewOrderRequest(123.456, 100, "AAPL", 1999, 1,  // NOLINT
    //                        FIX::Side_SELL);                // NOLINT
    //
    //    SendNewOrderRequest(123.456, 100, "AAPL", 1999, 1,              //
    //    NOLINT
    //                        FIX::Side_BUY);                             //
    //                        NOLINT
    //    SendNewOrderRequest(123.456, 50, "AAPL", 1999, 1,               //
    //    NOLINT
    //                        FIX::Side_SELL);                            //
    //                        NOLINT
    //    SendCancelOrderRequest(100, "AAPL", 1999, 1, FIX::Side_BUY, 4,  //
    //    NOLINT
    //                           "00000005");                             //
    //                           NOLINT
    //
    //    SendNewOrderRequest(123.456, 100, "AAPL", 1999, 1,     // NOLINT
    //                        FIX::Side_BUY);                    // NOLINT
    //    SendModifyOrderRequest(123.456, 50, "AAPL", 1999, 1,   // NOLINT
    //                           FIX::Side_BUY, 6, "00000008");  // NOLINT
    //
    /*
SendCancelOrderRequest(const std::int32_t& quantity,
                              const std::string& symbol,
                              const std::uint32_t& instrument_id,
                              const std::uint32_t& account_id,
                              const FIX::Side& side,
                              const std::uint32_t& order_id,
                              const std::string& orig_clord_id)

    SendNewOrderRequest(123.455, 21, "AAPL", 1999, 1,
                        FIX::Side_SELL);  // NOLINT
    SendNewOrderRequest(123.456, 22, "AAPL", 1999, 1,
                        FIX::Side_SELL);                             // NOLINT
    SendNewOrderRequest(123.46, 1, "AAPL", 1999, 1, FIX::Side_BUY);  // NOLINT
    */
  }

  auto onLogout(const FIX::SessionID& session_id) -> void override {
    spdlog::info("session logout: {}", session_id.toString());
  }

  auto toAdmin(FIX::Message& /*unused*/, const FIX::SessionID& /*unused*/)
      -> void override {}

  auto toApp(FIX::Message& /*unused*/, const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::DoNotSend) -> void override {}

  auto fromAdmin(const FIX::Message& /*unused*/,
                 const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::RejectLogon) -> void override {}

  auto fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::UnsupportedMessageType)
          -> void override {
    crack(message, sessionID);
  }

  auto onMessage(const FIX42::ExecutionReport& message,
                 const FIX::SessionID& /*unused*/) -> void override {
    FIX::OrderID order_id;
    FIX::ExecID exec_id;
    FIX::ExecTransType exec_trans_type;
    FIX::OrdStatus ord_status;
    FIX::Symbol symbol;
    FIX::Side side;
    FIX::OrderQty order_qty;
    FIX::LeavesQty leaves_qty;
    FIX::CumQty cum_qty;
    FIX::Price px;
    FIX::AvgPx avg_px;
    FIX::ClOrdID clord_id;
    FIX::OrigClOrdID orig_clord_id;
    FIX::SecurityID security_id;

    message.get(order_id);
    message.get(exec_id);
    message.get(exec_trans_type);
    message.get(ord_status);
    message.get(symbol);
    message.get(side);
    message.get(order_qty);
    message.get(leaves_qty);
    message.get(cum_qty);
    message.get(px);
    message.get(avg_px);
    message.get(clord_id);
    message.get(security_id);
    message.getIfSet(orig_clord_id);

    spdlog::info(
        "onMessage: ExecutionReport order_id {}, exec_id {}, security_id {}, "
        "ord_status {}, side {}, order_qty {}, leaves_qty {}, cum_qty {}, "
        "px {}, avg_px {}, clord_id {}, orig_clord_id {}",
        order_id.getString(), exec_id.getString(), security_id.getString(),
        ord_status.getString(), side.getString(), order_qty.getString(),
        leaves_qty.getString(), cum_qty.getString(), px.getString(),
        avg_px.getString(), clord_id.getString(), orig_clord_id.getString());
  }

  auto onMessage(const FIX42::OrderCancelReject& message,
                 const FIX::SessionID& /*unused*/) -> void override {
    spdlog::info("onMessage::OrderCancelReject {}", message.toXML());
  }

  auto SendNewOrderRequest(const double& price, const std::int32_t& quantity,
                           const std::string& symbol,
                           const std::uint32_t& instrument_id,
                           const std::uint32_t& account_id,
                           const FIX::Side& side) -> void {
    FIX42::NewOrderSingle newOrderSingle(
        FIX::ClOrdID(MakeClientOrderId(kClientOrderIdSize)),
        FIX::HandlInst(kHandleInstr), FIX::Symbol(symbol), FIX::Side(side),
        FIX::TransactTime(), FIX::OrdType(FIX::OrdType_LIMIT));

    newOrderSingle.setField(FIX::OrderQty(quantity));
    newOrderSingle.setField(FIX::Price(price));
    newOrderSingle.setField(FIX::Account(std::to_string(account_id)));
    newOrderSingle.setField(FIX::SecurityID(std::to_string(instrument_id)));
    newOrderSingle.setField(
        FIX::SecurityIDSource(FIX::SecurityIDSource_EXCHANGE_SYMBOL));
    newOrderSingle.setField(FIX::TimeInForce(FIX::TimeInForce_DAY));

    FIX::Session::sendToTarget(newOrderSingle, session_id_);
  }

  auto SendModifyOrderRequest(const double& price, const std::int32_t& quantity,
                              const std::string& symbol,
                              const std::uint32_t& instrument_id,
                              const std::uint32_t& account_id,
                              const FIX::Side& side,
                              const std::uint32_t& order_id,
                              const std::string& orig_clord_id) -> void {
    FIX42::OrderCancelReplaceRequest cancelReplaceRequest(
        FIX::OrigClOrdID(orig_clord_id),
        FIX::ClOrdID(MakeClientOrderId(kClientOrderIdSize)),
        FIX::HandlInst(kHandleInstr), FIX::Symbol(symbol), FIX::Side(side),
        FIX::TransactTime(), FIX::OrdType(FIX::OrdType_LIMIT));

    cancelReplaceRequest.setField(FIX::OrderQty(quantity));
    cancelReplaceRequest.setField(FIX::Price(price));
    cancelReplaceRequest.setField(FIX::OrderID(std::to_string(order_id)));
    cancelReplaceRequest.setField(FIX::Account(std::to_string(account_id)));
    cancelReplaceRequest.setField(
        FIX::SecurityID(std::to_string(instrument_id)));
    cancelReplaceRequest.setField(
        FIX::SecurityIDSource(FIX::SecurityIDSource_EXCHANGE_SYMBOL));

    FIX::Session::sendToTarget(cancelReplaceRequest, session_id_);
  }

  auto SendCancelOrderRequest(const std::int32_t& quantity,
                              const std::string& symbol,
                              const std::uint32_t& instrument_id,
                              const std::uint32_t& account_id,
                              const FIX::Side& side,
                              const std::uint32_t& order_id,
                              const std::string& orig_clord_id) -> void {
    FIX42::OrderCancelRequest orderCancelRequest(
        FIX::OrigClOrdID(orig_clord_id),
        FIX::ClOrdID(MakeClientOrderId(kClientOrderIdSize)),
        FIX::Symbol(symbol), FIX::Side(side), FIX::TransactTime());

    orderCancelRequest.setField(FIX::SecurityID(std::to_string(instrument_id)));
    orderCancelRequest.setField(
        FIX::SecurityIDSource(FIX::SecurityIDSource_EXCHANGE_SYMBOL));
    orderCancelRequest.setField(FIX::OrderID(std::to_string(order_id)));
    orderCancelRequest.setField(FIX::Account(std::to_string(account_id)));
    orderCancelRequest.setField(FIX::OrderQty(quantity));

    FIX::Session::sendToTarget(orderCancelRequest, session_id_);
  }

 private:
  std::string config_;
  std::unique_ptr<FIX::Initiator> initiator_;
  FIX::SessionID session_id_;
};

auto main(int argc, char** argv) -> int {
  if (argc < 2) {
    std::cout << "usage: " << argv[0] << " FILE." << std::endl;
    return 1;
  }

  std::string file = argv[1];
  spdlog::info("client config file: {}", file);

  FixClient client(file);
  client.Initialize();
  client.Start();
  client.Stop();

  return 0;
}
