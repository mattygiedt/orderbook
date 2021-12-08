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
    SendNewOrderSingle(123.456, 42, "AAPL", 1999, 1, FIX::Side_BUY);   // NOLINT
    SendNewOrderSingle(123.455, 21, "AAPL", 1999, 1, FIX::Side_SELL);  // NOLINT
    SendNewOrderSingle(123.456, 22, "AAPL", 1999, 1, FIX::Side_SELL);  // NOLINT
    SendNewOrderSingle(123.46, 1, "AAPL", 1999, 1, FIX::Side_BUY);     // NOLINT
  }

  auto onLogout(const FIX::SessionID& session_id) -> void override {
    spdlog::info("session logout: {}", session_id.toString());
  }

  auto toAdmin(FIX::Message& message, const FIX::SessionID& /*unused*/)
      -> void override {
    spdlog::info("toAdmin: {}", message.toString());
  }

  auto toApp(FIX::Message& message, const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::DoNotSend) -> void override {
    spdlog::info("toApp: {}", message.toString());
  }

  auto fromAdmin(const FIX::Message& message, const FIX::SessionID& /*unused*/)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::RejectLogon) -> void override {
    spdlog::info("fromAdmin: {}", message.toString());
  }

  auto fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::UnsupportedMessageType)
          -> void override {
    crack(message, sessionID);
  }

  auto onMessage(const FIX42::ExecutionReport& message,
                 const FIX::SessionID& /*unused*/) -> void override {
    spdlog::info("onMessage::ExecutionReport {}", message.toXML());
  }

  auto onMessage(const FIX42::OrderCancelReject& message,
                 const FIX::SessionID& /*unused*/) -> void override {
    spdlog::info("onMessage::OrderCancelReject {}", message.toXML());
  }

  auto SendNewOrderSingle(const double& price, const std::uint32_t& quantity,
                          const std::string& symbol,
                          const std::uint32_t& instrument_id,
                          const std::uint32_t& account_id,
                          const FIX::Side& side) -> void {
    FIX42::NewOrderSingle newOrderSingle(
        FIX::ClOrdID(MakeClientOrderId(kClientOrderIdSize)),
        FIX::HandlInst(
            FIX::
                HandlInst_AUTOMATED_EXECUTION_ORDER_PRIVATE_NO_BROKER_INTERVENTION),
        FIX::Symbol(symbol), FIX::Side(side), FIX::TransactTime(),
        FIX::OrdType(FIX::OrdType_LIMIT));

    newOrderSingle.setField(FIX::OrderQty(quantity));
    newOrderSingle.setField(FIX::Price(price));
    newOrderSingle.setField(FIX::Account(std::to_string(account_id)));
    newOrderSingle.setField(FIX::SecurityID(std::to_string(instrument_id)));
    newOrderSingle.setField(
        FIX::SecurityIDSource(FIX::SecurityIDSource_EXCHANGE_SYMBOL));
    newOrderSingle.setField(FIX::TimeInForce(FIX::TimeInForce_DAY));

    FIX::Session::sendToTarget(newOrderSingle, session_id_);
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
